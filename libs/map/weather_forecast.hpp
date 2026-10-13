#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace weather
{
// Point weather forecast from the Open-Meteo forecast API (https://api.open-meteo.com/v1/forecast).
// Keyless, no account, CC-BY 4.0 — chosen for the privacy-first app (no API key / tracking).
//
// Pure data + parsing: no networking here. Framework::GetWeatherForecast does the HTTP fetch and
// calls ParseForecast on the response.

struct CurrentConditions
{
  int64_t m_timeUnix = 0;          // observation time, seconds since epoch (UTC)
  double m_temperatureC = 0.0;     // 2 m air temperature
  double m_apparentTemperatureC = 0.0;
  double m_precipitationMm = 0.0;  // precipitation in the last hour
  int m_weatherCode = 0;           // WMO weather interpretation code
  double m_windSpeedKmh = 0.0;     // 10 m wind speed
  double m_windDirectionDeg = 0.0; // 10 m wind direction (0 = from north)
};

struct HourlyPoint
{
  int64_t m_timeUnix = 0;
  double m_temperatureC = 0.0;
  double m_precipitationMm = 0.0;
  double m_windSpeedKmh = 0.0;
  int m_weatherCode = 0;
};

struct Forecast
{
  CurrentConditions m_current;
  std::vector<HourlyPoint> m_hourly;
};

// Builds the Open-Meteo request URL for a WGS84 point: current conditions + 24 hourly steps,
// metric units, unix timestamps.
std::string BuildForecastUrl(double latDeg, double lonDeg);

// Parses an Open-Meteo forecast response. Returns false (leaving `out` untouched) on malformed
// JSON or when the hourly arrays are absent / of mismatched lengths.
bool ParseForecast(std::string const & json, Forecast & out);
}  // namespace weather
