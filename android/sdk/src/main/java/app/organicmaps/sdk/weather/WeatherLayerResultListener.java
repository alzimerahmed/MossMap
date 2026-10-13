package app.organicmaps.sdk.weather;

import androidx.annotation.Keep;

/// Callback for Framework.nativeSetWeatherLayerEnabled. Invoked on the UI thread.
@Keep
public interface WeatherLayerResultListener
{
  /// true when the weather layer became active; false on network/parse failure (toggle not persisted).
  void onResult(boolean success);
}
