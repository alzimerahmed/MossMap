package app.organicmaps.sdk.weather;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/// Point weather forecast fetched from Open-Meteo (keyless, CC-BY 4.0).
/// All temperatures in °C, precipitation in mm, wind speed in km/h, times as unix seconds (UTC).
/// Constructed from JNI; `current` = {time, temperature, apparentTemperature, precipitation,
/// weatherCode, windSpeed, windDirection}.
@Keep
public class WeatherForecast
{
  @NonNull
  public final double[] mCurrent;
  @NonNull
  public final double[] mHourlyTime;
  @NonNull
  public final double[] mHourlyTemperature;
  @NonNull
  public final double[] mHourlyPrecipitation;
  @NonNull
  public final double[] mHourlyWindSpeed;

  public WeatherForecast(@NonNull double[] current, @NonNull double[] hourlyTime,
                         @NonNull double[] hourlyTemperature, @NonNull double[] hourlyPrecipitation,
                         @NonNull double[] hourlyWindSpeed)
  {
    mCurrent = current;
    mHourlyTime = hourlyTime;
    mHourlyTemperature = hourlyTemperature;
    mHourlyPrecipitation = hourlyPrecipitation;
    mHourlyWindSpeed = hourlyWindSpeed;
  }
}
