#include "testing/testing.hpp"

#include "map/elevation_info.hpp"

#include <vector>

namespace elevation_info_tests
{
using ClimbSegment = ElevationInfo::ClimbSegment;

ElevationInfo MakeProfile(std::vector<std::pair<double, double>> const & distanceAltitude)
{
  // Build via Assign(): segDistances (N-1) + altitudes (N).
  std::vector<double> segDistances;
  geometry::Altitudes altitudes;
  altitudes.reserve(distanceAltitude.size());
  for (size_t i = 0; i < distanceAltitude.size(); ++i)
  {
    if (i > 0)
      segDistances.push_back(distanceAltitude[i].first);
    altitudes.push_back(static_cast<geometry::Altitude>(distanceAltitude[i].second));
  }
  ElevationInfo info;
  info.Assign(segDistances, altitudes);
  return info;
}

UNIT_TEST(ClimbSegments_SingleClimb)
{
  // 0 m @100 -> 1000 m @300 (gain 200) -> 1200 m @280 (descent).
  auto const info = MakeProfile({{0.0, 100}, {500.0, 150}, {1000.0, 300}, {1200.0, 280}});
  auto const segments = info.CalculateClimbSegments(25 /* minGainMeters */);

  TEST_EQUAL(segments.size(), 1, ());
  auto const & seg = segments[0];
  TEST_ALMOST_EQUAL_ULPS(seg.m_startDistance, 0.0, ());
  TEST_ALMOST_EQUAL_ULPS(seg.m_endDistance, 1000.0, ());
  TEST_EQUAL(seg.m_startAltitude, 100, ());
  TEST_EQUAL(seg.m_endAltitude, 300, ());
  TEST_EQUAL(seg.m_gain, 200, ());
  // Steepest sub-segment: 500 m run, 150 m gain -> 0.3.
  TEST_ALMOST_EQUAL_ULPS(seg.m_maxGradient, 0.3, ());
}

UNIT_TEST(ClimbSegments_MultipleClimbs)
{
  // Climb 1: 0..400 (gain 100), descent to 600, climb 2: 600..1000 (gain 80), descent.
  auto const info = MakeProfile({{0.0, 100}, {400.0, 200}, {600.0, 150}, {1000.0, 230}, {1100.0, 220}});
  auto const segments = info.CalculateClimbSegments(25);

  TEST_EQUAL(segments.size(), 2, ());
  TEST_EQUAL(segments[0].m_gain, 100, ());
  TEST_EQUAL(segments[1].m_gain, 80, ());
  TEST_ALMOST_EQUAL_ULPS(segments[1].m_startDistance, 600.0, ());
  TEST_ALMOST_EQUAL_ULPS(segments[1].m_endDistance, 1000.0, ());
}

UNIT_TEST(ClimbSegments_NoiseFiltered)
{
  // Tiny 10 m bumps must not be reported as climbs with the default threshold.
  auto const info = MakeProfile({{0.0, 100}, {100.0, 110}, {200.0, 100}, {300.0, 108}, {400.0, 100}});
  TEST(info.CalculateClimbSegments(25).empty(), ());
}

UNIT_TEST(ClimbSegments_Empty)
{
  ElevationInfo info;
  TEST(info.CalculateClimbSegments().empty(), ());
}

UNIT_TEST(ClimbSegments_FlatEndsInClimb)
{
  // Climb that reaches the end of the profile must still be reported.
  auto const info = MakeProfile({{0.0, 100}, {300.0, 200}});
  auto const segments = info.CalculateClimbSegments(25);
  TEST_EQUAL(segments.size(), 1, ());
  TEST_EQUAL(segments[0].m_gain, 100, ());
  TEST_ALMOST_EQUAL_ULPS(segments[0].m_endDistance, 300.0, ());
}

}  // namespace elevation_info_tests
