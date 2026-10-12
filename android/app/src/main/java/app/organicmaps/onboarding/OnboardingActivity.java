package app.organicmaps.onboarding;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.DownloadResourcesLegacyActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.MwmActivity;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.downloader.DownloaderActivity;
import app.organicmaps.downloader.MapManagerHelper;
import app.organicmaps.intent.Factory;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.progressindicator.LinearProgressIndicator;
import java.util.List;

/**
 * Guided first-run onboarding: shows the detected region, offers its map download with the size,
 * and explains what the app can do offline.
 * Shown exactly once, before {@link DownloadResourcesLegacyActivity} handles the
 * mandatory world-overview resources and the handoff to the map.
 */
public class OnboardingActivity extends BaseMwmFragmentActivity
{
  private static final String PREFS_NAME = "onboarding";
  private static final String KEY_ONBOARDING_SHOWN = "OnboardingShown";

  private TextView mRegionName;
  private TextView mRegionSize;
  private LinearProgressIndicator mProgress;
  private MaterialButton mBtnPrimary;
  private MaterialButton mBtnBrowseMaps;
  private View mBtnSkip;

  @Nullable
  private String mCountryId;
  private boolean mDownloadStarted;
  private boolean mLeaving;

  @NonNull
  private ActivityResultLauncher<Intent> mApiRequest;
  @NonNull
  private ActivityResultLauncher<Intent> mDownloaderRequest;

  private int mCountryDownloadListenerSlot;

  public static boolean isOnboardingPending(@NonNull Context context)
  {
    return !context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
                        .getBoolean(KEY_ONBOARDING_SHOWN, false);
  }

  private static void setOnboardingShown(@NonNull Context context)
  {
    context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        .edit()
        .putBoolean(KEY_ONBOARDING_SHOWN, true)
        .apply();
  }

  private final LocationListener mLocationListener = new LocationListener() {
    @Override
    public void onLocationUpdated(@NonNull android.location.Location location)
    {
      if (mCountryId != null)
        return;

      final String id = MapManager.nativeFindCountry(location.getLatitude(), location.getLongitude());
      if (TextUtils.isEmpty(id))
        return;

      mCountryId = id;
      MwmApplication.from(OnboardingActivity.this).getLocationHelper().removeListener(this);
      showSuggestedRegion();
    }
  };

  private final MapManager.StorageCallback mCountryDownloadListener = new MapManager.StorageCallback() {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      for (MapManager.StorageCallbackData item : data)
      {
        if (!item.isLeafNode)
          continue;

        switch (item.newStatus)
        {
        case CountryItem.STATUS_DONE:
          if (mCountryId != null && mCountryId.equals(item.countryId))
            proceedToMap();
          return;

        case CountryItem.STATUS_FAILED:
          // Let the user retry from the downloader or skip; keep the onboarding screen usable.
          mDownloadStarted = false;
          UiUtils.hide(mProgress);
          mBtnPrimary.setText(R.string.onboarding_download_region_button);
          MapManagerHelper.showError(OnboardingActivity.this, item, null);
          return;
        }
      }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (remoteSize > 0)
        mProgress.setProgressCompat((int) (100 * localSize / remoteSize), true);
    }
  };

  @CallSuper
  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    setContentView(R.layout.activity_onboarding);
    final View view = getWindow().getDecorView().findViewById(android.R.id.content);
    ViewCompat.setOnApplyWindowInsetsListener(view, PaddingInsetsListener.allSides());

    mRegionName = findViewById(R.id.onboarding_region_name);
    mRegionSize = findViewById(R.id.onboarding_region_size);
    mProgress = findViewById(R.id.onboarding_progress);
    mBtnPrimary = findViewById(R.id.onboarding_btn_primary);
    mBtnBrowseMaps = findViewById(R.id.onboarding_btn_browse_maps);
    mBtnSkip = findViewById(R.id.onboarding_btn_skip);

    mApiRequest = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), result -> {
      setResult(result.getResultCode(), result.getData());
      finish();
    });
    mDownloaderRequest = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
                                                   result -> proceedToMap());

    mBtnPrimary.setOnClickListener(v -> onPrimaryClicked());
    mBtnBrowseMaps.setOnClickListener(v -> startBrowserActivity());
    mBtnSkip.setOnClickListener(v -> {
      setOnboardingShown(this);
      proceedToMap();
    });

    showFallbackState();
    if (LocationUtils.checkLocationPermission(this))
    {
      final LocationHelper locationHelper = MwmApplication.from(this).getLocationHelper();
      locationHelper.addListener(mLocationListener);
      if (!locationHelper.isActive())
        locationHelper.start();
    }
  }

  @CallSuper
  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    mApiRequest.unregister();
    mApiRequest = null;
    mDownloaderRequest.unregister();
    mDownloaderRequest = null;
    if (mCountryDownloadListenerSlot != 0)
    {
      MapManager.nativeUnsubscribe(mCountryDownloadListenerSlot);
      mCountryDownloadListenerSlot = 0;
    }
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    MwmApplication.from(this).getLocationHelper().removeListener(mLocationListener);
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    if (mCountryId == null && LocationUtils.checkLocationPermission(this))
      MwmApplication.from(this).getLocationHelper().addListener(mLocationListener);
  }

  /**
   * Same contract as {@link DownloadResourcesLegacyActivity}: our intent may carry a
   * payload (e.g. a shared file) that {@link MwmActivity} acts on, so say explicitly
   * that its state is unknown when a core restart is needed.
   */
  @Override
  protected void prepareIntentForCoreRestart(@NonNull Intent intent, @Nullable Bundle savedInstanceState)
  {
    if (!intent.hasExtra(MwmActivity.EXTRA_CONSUMED))
      intent.putExtra(MwmActivity.EXTRA_CONSUMED, false);
  }

  private void showFallbackState()
  {
    // No location permission or no fix yet: suggest the downloader instead of a specific region.
    mRegionName.setText(R.string.onboarding_no_location_title);
    mRegionSize.setText(R.string.onboarding_no_location_hint);
    mBtnPrimary.setText(R.string.onboarding_continue_button);
    UiUtils.show(mBtnBrowseMaps);
  }

  private void showSuggestedRegion()
  {
    final CountryItem item = CountryItem.fill(mCountryId);
    mRegionName.setText(item.name);
    UiUtils.hide(mBtnBrowseMaps);
    if (item.status == CountryItem.STATUS_DONE)
    {
      mRegionSize.setText(R.string.onboarding_region_already_downloaded);
      mBtnPrimary.setText(R.string.onboarding_continue_button);
      return;
    }

    mRegionSize.setText(getString(R.string.onboarding_region_size,
                                  StringUtils.getFileSizeString(this, item.totalSize)));
    mBtnPrimary.setText(R.string.onboarding_download_region_button);
  }

  private void onPrimaryClicked()
  {
    if (mCountryId == null || mDownloadStarted)
    {
      setOnboardingShown(this);
      proceedToMap();
      return;
    }

    setOnboardingShown(this);
    if (CountryItem.fill(mCountryId).status == CountryItem.STATUS_DONE)
    {
      proceedToMap();
      return;
    }

    mDownloadStarted = true;
    mBtnPrimary.setText(R.string.onboarding_downloading);
    // Announce the state change for screen readers (WCAG 4.1.3 status messages).
    mBtnPrimary.announceForAccessibility(getString(R.string.onboarding_downloading));
    UiUtils.show(mProgress);
    mCountryDownloadListenerSlot = MapManager.nativeSubscribe(mCountryDownloadListener);
    MapManagerHelper.startDownload(this, mCountryId);
  }

  private void startBrowserActivity()
  {
    setOnboardingShown(this);
    mDownloaderRequest.launch(new Intent(this, DownloaderActivity.class));
  }

  /**
   * Hands the original intent over to the legacy resources screen, which downloads the mandatory
   * world-overview resources (if any) and then continues to the map. Re-using the original intent
   * retains all flags and payload.
   */
  private void proceedToMap()
  {
    if (mLeaving)
      return;
    mLeaving = true;

    final Intent intent = getIntent();
    intent.setComponent(new ComponentName(this, DownloadResourcesLegacyActivity.class));
    intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_CLEAR_TOP);

    // See SplashActivity.processNavigation(): API callers wait for a result from MwmActivity.
    if (Factory.isStartedForApiResult(intent))
    {
      mApiRequest.launch(intent);
      return;
    }

    startActivity(intent);
    finish();
  }
}
