package app.organicmaps.sdk.weather;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/// Callback for Framework.nativeGetWeatherForecast. Invoked on the UI thread: on success
/// `forecast` is non-null and `error` is null; on failure `error` is non-null.
@Keep
public interface WeatherForecastListener
{
  void onForecast(@Nullable WeatherForecast forecast, @Nullable String error);
}
