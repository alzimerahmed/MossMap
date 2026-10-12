#include "testing/testing.hpp"

#include "map/framework.hpp"

#include <string>

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
}

UNIT_TEST(BackgroundTilesURL_MissingPlaceholders)
{
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/{z}/{x}"), ());
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/{z}/{x}/{Y}"), ());
  // Percent-encoded braces do not count: the provider substitutes literal {z}/{x}/{y}.
  TEST(!Framework::IsWellFormedBackgroundTilesURL("https://h/%7Bz%7D/%7Bx%7D/%7By%7D"), ());
}
}  // namespace
