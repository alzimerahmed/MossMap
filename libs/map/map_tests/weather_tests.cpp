#include "testing/testing.hpp"

#include "map/weather_forecast.hpp"
#include "map/weather_layer.hpp"

#include <string>

namespace weather_tests
{
using namespace std;
using namespace weather;

char const kForecastJson[] = R"({
  "latitude": 52.52, "longitude": 13.41, "timezone": "UTC",
  "current": {
    "time": 1700000000, "interval": 900,
    "temperature_2m": 12.3, "apparent_temperature": 10.1,
    "precipitation": 0.4, "weather_code": 61,
    "wind_speed_10m": 17.2, "wind_direction_10m": 245
  },
  "hourly": {
    "time": [1700000000, 1700003600, 1700007200],
    "temperature_2m": [12.3, 11.8, 11.1],
    "precipitation": [0.4, 0.0, 0.7],
    "wind_speed_10m": [17.2, 15.5, 13.0],
    "weather_code": [61, 3, 80]
  }
})";

UNIT_TEST(ForecastUrl)
{
  string const url = BuildForecastUrl(52.52, 13.405);
  TEST(url.starts_with("https://api.open-meteo.com/v1/forecast?latitude=52.5200&longitude=13.4050"), (url));
  TEST(url.find("current=temperature_2m") != string::npos, (url));
  TEST(url.find("timeformat=unixtime") != string::npos, (url));
}

UNIT_TEST(ForecastParseOk)
{
  Forecast f;
  TEST(ParseForecast(kForecastJson, f), ());

  TEST_EQUAL(f.m_current.m_timeUnix, 1700000000, ());
  TEST_ALMOST_EQUAL_ULPS(f.m_current.m_temperatureC, 12.3, ());
  TEST_ALMOST_EQUAL_ULPS(f.m_current.m_apparentTemperatureC, 10.1, ());
  TEST_ALMOST_EQUAL_ULPS(f.m_current.m_precipitationMm, 0.4, ());
  TEST_EQUAL(f.m_current.m_weatherCode, 61, ());
  TEST_ALMOST_EQUAL_ULPS(f.m_current.m_windSpeedKmh, 17.2, ());
  TEST_ALMOST_EQUAL_ULPS(f.m_current.m_windDirectionDeg, 245.0, ());

  TEST_EQUAL(f.m_hourly.size(), 3, ());
  TEST_EQUAL(f.m_hourly[2].m_timeUnix, 1700007200, ());
  TEST_ALMOST_EQUAL_ULPS(f.m_hourly[2].m_precipitationMm, 0.7, ());
  TEST_EQUAL(f.m_hourly[2].m_weatherCode, 80, ());
}

UNIT_TEST(ForecastParseFailures)
{
  Forecast f;
  // Mismatched hourly array lengths must be rejected.
  char const kMismatched[] = R"({
    "current": {"time": 1, "temperature_2m": 1.0, "apparent_temperature": 1.0, "precipitation": 0.0,
                "weather_code": 0, "wind_speed_10m": 0.0, "wind_direction_10m": 0.0},
    "hourly": {"time": [1, 2], "temperature_2m": [1.0], "precipitation": [0.0, 0.0],
               "wind_speed_10m": [0.0, 0.0], "weather_code": [0, 0]}
  })";
  TEST(!ParseForecast(kMismatched, f), ());

  TEST(!ParseForecast("not json at all", f), ());
  TEST(!ParseForecast("", f), ());

  // Missing hourly block entirely.
  TEST(!ParseForecast(R"({"current": {"time": 1}})", f), ());
}

UNIT_TEST(RadarMetaParse)
{
  char const kMeta[] = R"({
    "version": 2, "generated": 1700000000,
    "host": "https://tilecache.rainviewer.com",
    "radar": {
      "past": [
        {"time": 1699998800, "path": "/v2/radar/1699998800"},
        {"time": 1700000000, "path": "/v2/radar/1700000000"}
      ],
      "nowcast": [
        {"time": 1700001200, "path": "/v2/radar/nowcast_1700001200"}
      ]
    },
    "satellite": {"infrared": []}
  })";

  RadarFrame frame;
  TEST(ParseRadarMeta(kMeta, frame), ());
  // The latest *past* frame is used, not the nowcast.
  TEST_EQUAL(frame.m_timeUnix, 1700000000, ());
  TEST_EQUAL(frame.m_path, "/v2/radar/1700000000", ());

  string const tmpl = BuildRadarTileUrlTemplate(frame.m_path);
  TEST_EQUAL(tmpl, "https://tilecache.rainviewer.com/v2/radar/1700000000/256/{z}/{x}/{y}/2/1_1.png", (tmpl));
}

UNIT_TEST(RadarMetaFailures)
{
  RadarFrame frame;
  TEST(!ParseRadarMeta("", frame), ());
  TEST(!ParseRadarMeta("{}", frame), ());
  TEST(!ParseRadarMeta(R"({"radar": {"past": []}})", frame), ());
  TEST(!ParseRadarMeta("garbage", frame), ());
}
}  // namespace weather_tests
