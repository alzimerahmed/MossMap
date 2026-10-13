#include "map/weather_layer.hpp"

#include "base/logging.hpp"

#include <glaze/json.hpp>

#include <string>
#include <vector>

namespace weather
{
// Named namespace at external linkage: glaze's static reflection requires the type to have
// linkage (anonymous-namespace types break its name template).
namespace radar_json
{
struct FrameJson
{
  int64_t time = 0;
  std::string path;
};

struct RadarJson
{
  std::vector<FrameJson> past;
};

struct MetaJson
{
  RadarJson radar;
};
}  // namespace radar_json

bool ParseRadarMeta(std::string const & json, RadarFrame & out)
{
  if (json.empty())
    return false;

  radar_json::MetaJson parsed;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  auto const error = glz::read<opts>(parsed, json);
  if (error)
  {
    LOG(LWARNING, ("weather: failed to parse radar metadata JSON:", glz::format_error(error, json)));
    return false;
  }

  if (parsed.radar.past.empty())
  {
    LOG(LWARNING, ("weather: radar metadata contains no past frames"));
    return false;
  }

  radar_json::FrameJson const & latest = parsed.radar.past.back();
  if (latest.path.empty())
    return false;

  out.m_timeUnix = latest.time;
  out.m_path = latest.path;
  return true;
}

std::string BuildRadarTileUrlTemplate(std::string const & framePath)
{
  // https://www.rainviewer.com/api/weather-maps-api.html:
  // {host}{path}/{size}/{z}/{x}/{y}/{color}/{smooth}_{snow}.png
  return "https://tilecache.rainviewer.com" + framePath + "/256/{z}/{x}/{y}/2/1_1.png";
}
}  // namespace weather
