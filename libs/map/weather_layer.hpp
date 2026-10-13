#pragma once

#include <cstdint>
#include <string>

namespace weather
{
// Built-in weather raster overlay source.
//
// Open-Meteo's weather maps use a custom binary OM-file protocol (om://), not plain XYZ PNGs, so
// they cannot be rendered by the existing RasterTileProvider. The keyless XYZ raster weather source
// used instead is RainViewer's public radar/nowcast tiles (no API key, no account; free tier):
//   metadata: https://api.rainviewer.com/public/weather-maps.json
//   tiles:    https://tilecache.rainviewer.com{path}/256/{z}/{x}/{y}/{color}/1_1.png
// Pure parsing/URL-building only; Framework owns the fetch + RasterTileProvider hookup.

struct RadarFrame
{
  int64_t m_timeUnix = 0;  // frame timestamp, seconds since epoch (UTC)
  std::string m_path;      // tile path, e.g. "/v2/radar/1666660000"
};

// Parses RainViewer's weather-maps.json and picks the latest available past radar frame.
// Returns false (leaving `out` untouched) on malformed JSON or when no past frame exists.
bool ParseRadarMeta(std::string const & json, RadarFrame & out);

// Builds the XYZ tile URL template ({z}/{x}/{y} placeholders intact) for a radar frame path.
// Color scheme 2 (universal blue), smoothed, no snow.
std::string BuildRadarTileUrlTemplate(std::string const & framePath);
}  // namespace weather
