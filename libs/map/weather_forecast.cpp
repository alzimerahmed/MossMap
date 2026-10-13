#include "map/weather_forecast.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <cstdio>

namespace weather
{
// Named namespace at external linkage: glaze's static reflection requires the type to have
// linkage (anonymous-namespace types break its name template).
namespace forecast_json
{
// Member names deliberately match the Open-Meteo JSON keys so glaze's static reflection maps
// them without a custom meta definition.
struct CurrentJson
{
  int64_t time = 0;
  double temperature_2m = 0.0;
  double apparent_temperature = 0.0;
  double precipitation = 0.0;
  int weather_code = 0;
  double wind_speed_10m = 0.0;
  double wind_direction_10m = 0.0;
};

struct HourlyJson
{
  std::vector<int64_t> time;
  std::vector<double> temperature_2m;
  std::vector<double> precipitation;
  std::vector<double> wind_speed_10m;
  std::vector<int> weather_code;
};

struct ForecastJson
{
  CurrentJson current;
  HourlyJson hourly;
};
}  // namespace forecast_json

std::string BuildForecastUrl(double latDeg, double lonDeg)
{
  // Clamp to the WGS84 domain so a degenerate map center cannot produce a 400.
  double const lat = std::clamp(latDeg, -90.0, 90.0);
  double const lon = std::clamp(lonDeg, -180.0, 180.0);

  char buf[512];
  std::snprintf(buf, sizeof(buf),
                "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
                "&current=temperature_2m,apparent_temperature,precipitation,weather_code,wind_speed_10m,"
                "wind_direction_10m"
                "&hourly=temperature_2m,precipitation,wind_speed_10m,weather_code"
                "&forecast_days=2&forecast_hours=24&timeformat=unixtime&wind_speed_unit=kmh",
                lat, lon);
  return buf;
}

bool ParseForecast(std::string const & json, Forecast & out)
{
  if (json.empty())
    return false;

  forecast_json::ForecastJson parsed;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  auto const error = glz::read<opts>(parsed, json);
  if (error)
  {
    LOG(LWARNING, ("weather: failed to parse forecast JSON:", glz::format_error(error, json)));
    return false;
  }

  auto const & times = parsed.hourly.time;
  size_t const n = times.size();
  if (n == 0 || parsed.hourly.temperature_2m.size() != n || parsed.hourly.precipitation.size() != n ||
      parsed.hourly.wind_speed_10m.size() != n || parsed.hourly.weather_code.size() != n)
  {
    LOG(LWARNING, ("weather: forecast hourly arrays absent or of mismatched lengths"));
    return false;
  }

  Forecast result;
  result.m_current.m_timeUnix = parsed.current.time;
  result.m_current.m_temperatureC = parsed.current.temperature_2m;
  result.m_current.m_apparentTemperatureC = parsed.current.apparent_temperature;
  result.m_current.m_precipitationMm = parsed.current.precipitation;
  result.m_current.m_weatherCode = parsed.current.weather_code;
  result.m_current.m_windSpeedKmh = parsed.current.wind_speed_10m;
  result.m_current.m_windDirectionDeg = parsed.current.wind_direction_10m;

  result.m_hourly.resize(n);
  for (size_t i = 0; i < n; ++i)
  {
    result.m_hourly[i].m_timeUnix = times[i];
    result.m_hourly[i].m_temperatureC = parsed.hourly.temperature_2m[i];
    result.m_hourly[i].m_precipitationMm = parsed.hourly.precipitation[i];
    result.m_hourly[i].m_windSpeedKmh = parsed.hourly.wind_speed_10m[i];
    result.m_hourly[i].m_weatherCode = parsed.hourly.weather_code[i];
  }

  out = std::move(result);
  return true;
}
}  // namespace weather
