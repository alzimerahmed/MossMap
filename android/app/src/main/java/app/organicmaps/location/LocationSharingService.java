package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ServiceInfo;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.NotificationChannelCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.app.ServiceCompat;
import androidx.core.content.ContextCompat;
import androidx.preference.PreferenceManager;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Opt-in live location sharing to a user-chosen endpoint (self-hosted or any HTTP collector).
 * Privacy-first: no third-party service, no server-side account, the endpoint URL and the
 * posting interval are configured by the user in Settings > Privacy. The device id is a random
 * UUID generated locally, so the endpoint cannot correlate the stream with anything but itself.
 */
public class LocationSharingService extends Service implements LocationListener
{
  public static final String LOCATION_SHARING_CHANNEL_ID = "LOCATION SHARING";
  private static final int LOCATION_SHARING_NOTIFICATION_ID = 54322;
  private static final String TAG = LocationSharingService.class.getSimpleName();

  private final HandlerThread mNetworkThread = new HandlerThread("LocationSharing-Net");
  private Handler mNetworkHandler;
  private final AtomicReference<String> mEndpoint = new AtomicReference<>();
  private String mDeviceId;
  private long mMinIntervalMs;

  @Nullable
  @Override
  public IBinder onBind(Intent intent)
  {
    return null;
  }

  public static boolean isEnabled(@NonNull Context context)
  {
    final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
    return prefs.getBoolean(context.getString(R.string.pref_location_sharing), false)
           && !prefs.getString(context.getString(R.string.pref_location_sharing_endpoint), "").isEmpty();
  }

  public static void start(@NonNull Context context)
  {
    if (!isEnabled(context))
      return;
    ContextCompat.startForegroundService(context, new Intent(context, LocationSharingService.class));
  }

  public static void stop(@NonNull Context context)
  {
    context.stopService(new Intent(context, LocationSharingService.class));
  }

  public static void createNotificationChannel(@NonNull Context context)
  {
    final NotificationChannelCompat channel =
        new NotificationChannelCompat.Builder(LOCATION_SHARING_CHANNEL_ID, NotificationManagerCompat.IMPORTANCE_LOW)
            .setName(context.getString(R.string.location_sharing_title))
            .setLightsEnabled(false)
            .setVibrationEnabled(false)
            .build();
    NotificationManagerCompat.from(context).createNotificationChannel(channel);
  }

  @Override
  public void onCreate()
  {
    mNetworkThread.start();
    mNetworkHandler = new Handler(mNetworkThread.getLooper());
    final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
    mDeviceId = prefs.getString(getString(R.string.pref_location_sharing_device_id), null);
    if (mDeviceId == null)
    {
      mDeviceId = UUID.randomUUID().toString();
      prefs.edit().putString(getString(R.string.pref_location_sharing_device_id), mDeviceId).apply();
    }
  }

  @Override
  public int onStartCommand(@NonNull Intent intent, int flags, int startId)
  {
    if (!MwmApplication.from(this).getOrganicMaps().arePlatformAndCoreInitialized())
    {
      stopSelf();
      return START_NOT_STICKY;
    }
    if (!isEnabled(this))
    {
      stopSelf();
      return START_NOT_STICKY;
    }
    if (checkSelfPermission(ACCESS_FINE_LOCATION) != PERMISSION_GRANTED)
    {
      Logger.w(TAG, "ACCESS_FINE_LOCATION not granted, stopping location sharing");
      stopSelf();
      return START_NOT_STICKY;
    }

    final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
    mEndpoint.set(prefs.getString(getString(R.string.pref_location_sharing_endpoint), ""));
    mMinIntervalMs = readIntervalMs(prefs);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
      ServiceCompat.startForeground(this, LOCATION_SHARING_NOTIFICATION_ID, buildNotification(),
                                    ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);
    else
      ServiceCompat.startForeground(this, LOCATION_SHARING_NOTIFICATION_ID, buildNotification(), 0);

    final LocationManager lm = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
    if (checkSelfPermission(ACCESS_FINE_LOCATION) == PERMISSION_GRANTED)
      lm.removeUpdates(this);
    // Passive provider piggybacks on other providers' fixes: no extra GPS duty cycles, so sharing
    // adds near-zero battery cost on top of whatever the app is already doing.
    lm.requestLocationUpdates(LocationManager.PASSIVE_PROVIDER, mMinIntervalMs, 0, this, getMainLooper());
    return START_NOT_STICKY;
  }

  private long readIntervalMs(@NonNull SharedPreferences prefs)
  {
    final String value = prefs.getString(getString(R.string.pref_location_sharing_interval), "10");
    try
    {
      return Long.parseLong(value) * 1000L;
    }
    catch (NumberFormatException e)
    {
      return 10_000L;
    }
  }

  private NotificationCompat.Builder getNotificationBuilder()
  {
    final Intent contentIntent = new Intent(this, MwmActivity.class);
    final int flagImmutable = Build.VERSION.SDK_INT < Build.VERSION_CODES.M ? 0 : PendingIntent.FLAG_IMMUTABLE;
    final PendingIntent pendingIntent =
        PendingIntent.getActivity(this, 0, contentIntent, PendingIntent.FLAG_UPDATE_CURRENT | flagImmutable);
    return new NotificationCompat.Builder(this, LOCATION_SHARING_CHANNEL_ID)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setVisibility(NotificationCompat.VISIBILITY_SECRET)
        .setOngoing(true)
        .setOnlyAlertOnce(true)
        .setSmallIcon(app.organicmaps.branding.R.drawable.ic_splash)
        .setContentTitle(getString(R.string.location_sharing_title))
        .setContentText(getString(R.string.location_sharing_notification_text))
        .setContentIntent(pendingIntent)
        .setColor(ContextCompat.getColor(this, R.color.notification));
  }

  private android.app.Notification buildNotification()
  {
    return getNotificationBuilder().build();
  }

  @Override
  public void onLocationChanged(@NonNull Location location)
  {
    final String endpoint = mEndpoint.get();
    if (endpoint == null || endpoint.isEmpty() || mNetworkHandler == null)
      return;
    final String payload = buildPayload(location);
    mNetworkHandler.post(() -> postLocation(endpoint, payload));
  }

  private String buildPayload(@NonNull Location location)
  {
    return String.format(Locale.US,
        "{\"lat\":%.6f,\"lon\":%.6f,\"accuracy\":%.1f,\"speed\":%.2f,\"ts\":%d,\"id\":\"%s\"}",
        location.getLatitude(), location.getLongitude(), location.getAccuracy(),
        location.getSpeed(), location.getTime(), mDeviceId);
  }

  private void postLocation(@NonNull String endpoint, @NonNull String payload)
  {
    HttpURLConnection connection = null;
    try
    {
      final URL url = new URL(endpoint);
      // The endpoint is user-chosen; both schemes are allowed so self-hosted LAN
      // collectors (http://192.168.x.x) work out of the box.
      final String scheme = url.getProtocol();
      if (!"http".equals(scheme) && !"https".equals(scheme))
        return;
      connection = (HttpURLConnection) url.openConnection();
      connection.setRequestMethod("POST");
      connection.setRequestProperty("Content-Type", "application/json");
      connection.setDoOutput(true);
      connection.setConnectTimeout(10_000);
      connection.setReadTimeout(10_000);
      try (OutputStream os = connection.getOutputStream())
      {
        os.write(payload.getBytes(StandardCharsets.UTF_8));
      }
      connection.getResponseCode();
    }
    catch (IOException e)
    {
      Logger.w(TAG, "Location sharing POST failed: " + e.getMessage());
    }
    finally
    {
      if (connection != null)
        connection.disconnect();
    }
  }

  @Override
  public void onDestroy()
  {
    final LocationManager lm = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
    if (checkSelfPermission(ACCESS_FINE_LOCATION) == PERMISSION_GRANTED)
      lm.removeUpdates(this);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
      mNetworkThread.quitSafely();
    super.onDestroy();
  }

  @Override
  public void onProviderEnabled(@NonNull String provider) {}

  @Override
  public void onProviderDisabled(@NonNull String provider) {}
}
