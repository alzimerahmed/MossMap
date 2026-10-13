#include "testing/testing.hpp"

#include "map/framework.hpp"

#include <string>
#include <vector>

namespace
{
// Pure-logic tests for the satellite imagery (custom raster background tiles) layer.
// Framework::IsWellFormedBackgroundTilesURL is static and side-effect free, so it can be
// exercised without constructing a Framework or a Drape engine.

UNIT_TEST(BackgroundTilesURL_WellFormed)
{
  TEST(Framework::IsWellFormedBackgroundTilesURL("https://tiles.example.com/{z}/{x}/{y}.png"), ());
  TEST(Framework::IsWellFormedBackgroundTilesURL("https://a.b.c/{z}/{x}/{y}"), ());
  // Placeholders may appear in any order and be embedded in larger path segments.
  TEST(Framework::IsWellFormedBackgroundTilesURL("https://h/x={x}&y={y}&z={z}"), ());
  // No path at all is still well-formed as long as the host is non-empty and the
  // placeholders are present (they would be appended by the provider).
  TEST(Framework::IsWellFormedBackgroundTilesURL("https://h/{z}/{x}/{y}"), ());
}

UNIT_TEST(BackgroundTilesURL_BadScheme)
{
  TEST(!Framework::IsWellFormedBackgroundTilesURL(""), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("ftp://h/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("//h/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https:/h/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("HTTPS://h/{z}/{x}/{y}"), ());
  // Plaintext http would allow trivial MITM map-image injection.
  TEST(!Framework::IsWellFormedBackgroundTilesURL("http://tiles.example.com/{z}/{x}/{y}.png"), ());
}

UNIT_TEST(BackgroundTilesURL_BadHost)
{
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https:///"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https:///{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("http://"), ());
  // Userinfo and internal/loopback hosts must be rejected (SSRF-lite hardening).
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://user:pass@tiles.example.com/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://localhost/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://127.0.0.1/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://10.0.0.1/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://192.168.1.1/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://[::1]/{z}/{x}/{y}"), ());
  // RFC1918 172.16.0.0/12 — including its upper half.
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://172.16.0.1/{z}/{x}/{y}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://172.31.255.1/{z}/{x}/{y}"), ());
  // Public 172.x addresses must stay allowed (no prefix-based over-blocking).
  TEST(Framework::IsWellFormedBackgroundTilesURL("https://172.32.0.1/{z}/{x}/{y}"), ());
  TEST(Framework::IsWellFormedBackgroundTilesURL("https://172.64.10.1/{z}/{x}/{y}"), ());
}

UNIT_TEST(BackgroundTilesURL_MissingPlaceholders)
{
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/{z}/{x}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/{z}/{x}/{Y}"), ());
  // Percent-encoded braces do not count: the provider substitutes literal {z}/{x}/{y}.
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/%7Bz%7D/%7Bx%7D/%7By%7D"), ());
}

UNIT_TEST(BackgroundTilesURL_WmsBBox)
{
  // Simple WMS GetMap URLs use the {bbox} placeholder instead of {z}/{x}/{y}.
  TEST(Framework::IsWellFormedBackgroundTilesURL(
      "https://wms.example.com/wms?service=WMS&version=1.3.0&request=GetMap&layers=lc&bbox={bbox}"
      "&width=256&height=256&crs=EPSG:3857&format=image/png"), ());
  TEST(Framework::IsWellFormedBackgroundTilesURL("https://h/?BBOX={bbox}"), ());
  // WMS URLs must still satisfy the scheme/host rules.
  TEST(!Framework::IsWellFormedBackgroundTilesURL("http://wms.example.com/?bbox={bbox}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://localhost/?bbox={bbox}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/?bbox=%7Bbbox%7D"), ());
  // Neither XYZ nor bbox placeholders: reject.
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/tiles.png"), ());
}

UNIT_TEST(BackgroundTilesSources_SerializeParseRoundTrip)
{
  std::vector<Framework::BackgroundTilesSource> const sources = {
      {"Satellite", "https://tiles.example.com/{z}/{x}/{y}.png", "xyz", 100},
      {"Land Cover", "https://wms.example.com/wms?bbox={bbox}&crs=EPSG:3857", "wms", 40},
      {"Weird \"name\" \\ with / chars", "https://h/{z}/{x}/{y}", "xyz", 0},
  };

  std::string const json = Framework::SerializeBackgroundTilesSources(sources);
  TEST(!json.empty(), ());

  std::vector<Framework::BackgroundTilesSource> parsed;
  TEST(Framework::ParseBackgroundTilesSources(json, parsed), ());
  TEST_EQUAL(parsed.size(), sources.size(), ());
  for (size_t i = 0; i < sources.size(); ++i)
  {
    TEST_EQUAL(parsed[i].mName, sources[i].mName, ());
    TEST_EQUAL(parsed[i].mUrl, sources[i].mUrl, ());
    TEST_EQUAL(parsed[i].mType, sources[i].mType, ());
    TEST_EQUAL(parsed[i].mOpacityPct, sources[i].mOpacityPct, ());
  }
}

UNIT_TEST(BackgroundTilesSources_ParseMalformed)
{
  std::vector<Framework::BackgroundTilesSource> parsed;
  TEST(!Framework::ParseBackgroundTilesSources("", parsed), ());
  TEST(!Framework::ParseBackgroundTilesSources("not json", parsed), ());
  TEST(!Framework::ParseBackgroundTilesSources("{\"name\":\"obj not array\"}", parsed), ());
  TEST(parsed.empty(), ());
}

UNIT_TEST(BackgroundTilesSources_ParseClampsOpacity)
{
  std::vector<Framework::BackgroundTilesSource> parsed;
  TEST(Framework::ParseBackgroundTilesSources(
      R"([{"name":"a","url":"https://h/{z}/{x}/{y}","type":"xyz","opacity":999},
          {"name":"b","url":"https://h/{z}/{x}/{y}","type":"xyz","opacity":0}])", parsed), ());
  TEST_EQUAL(parsed.size(), 2, ());
  TEST_EQUAL(parsed[0].mOpacityPct, Framework::kBackgroundTilesMaxAreaOpacityPct, ());
  TEST_EQUAL(parsed[1].mOpacityPct, Framework::kBackgroundTilesMinAreaOpacityPct, ());
}

UNIT_TEST(BackgroundTilesSources_ParseEmptyList)
{
  std::vector<Framework::BackgroundTilesSource> parsed;
  TEST(Framework::ParseBackgroundTilesSources("[]", parsed), ());
  TEST(parsed.empty(), ());
}
}  // namespace
