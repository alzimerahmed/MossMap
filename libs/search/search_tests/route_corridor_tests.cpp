#include "testing/testing.hpp"

#include "search/route_corridor.hpp"

#include "indexer/data_source.hpp"

#include "geometry/mercator.hpp"

#include <memory>

namespace search_tests
{
namespace
{
// A straight west-east route of ~100 km along the equator, in mercator points.
m2::PolylineD MakeTestRoute()
{
  m2::PointD const start = mercator::FromLatLon(0.0, 0.0);
  m2::PointD const end = mercator::FromLatLon(0.0, 1.0);
  return m2::PolylineD({start, end});
}
}  // namespace

UNIT_TEST(RouteCorridor_Contains)
{
  search::RouteCorridor corridor(MakeTestRoute(), search::RouteCorridor::kDefaultRadiusMeters);
  TEST(corridor.IsValid(), ());

  // A point on the route is at distance 0.
  m2::PointD const onRoute = mercator::FromLatLon(0.0, 0.5);
  TEST_ALMOST_EQUAL_ULPS(corridor.GetDistanceFromRouteMeters(onRoute), 0.0, ());

  // A point ~1 km north of the route midpoint is inside the 2 km corridor.
  m2::PointD const inside = mercator::FromLatLon(1.0 * search::RouteCorridor::kDefaultRadiusMeters / 111320.0, 0.5);
  TEST(corridor.Contains(inside), ());
  double const dInside = corridor.GetDistanceFromRouteMeters(inside);
  TEST_GREATER(dInside, 0.0, ());
  TEST_LESS(dInside, search::RouteCorridor::kDefaultRadiusMeters, ());

  // A point ~10 km north of the route is outside.
  m2::PointD const outside = mercator::FromLatLon(10000.0 / 111320.0, 0.5);
  TEST(!corridor.Contains(outside), ());
  TEST_GREATER(corridor.GetDistanceFromRouteMeters(outside), search::RouteCorridor::kDefaultRadiusMeters, ());
}

UNIT_TEST(RouteCorridor_EndpointProjection)
{
  // Points beyond the route ends project onto the endpoints (clamped).
  search::RouteCorridor corridor(MakeTestRoute(), 2000.0);
  m2::PointD const beyondStart = mercator::FromLatLon(0.0, -0.001);
  TEST(corridor.Contains(beyondStart), ());
  double const d = corridor.GetDistanceFromRouteMeters(beyondStart);
  TEST_GREATER(d, 0.0, ());
  TEST_LESS(d, 2000.0, ());
}

UNIT_TEST(RouteCorridor_Rect)
{
  search::RouteCorridor corridor(MakeTestRoute(), 2000.0);
  auto const & rect = corridor.GetRect();
  TEST(rect.IsPointInside(mercator::FromLatLon(0.0, 0.5)), ());
  TEST(rect.IsPointInside(mercator::FromLatLon(1500.0 / 111320.0, 0.5)), ());
  TEST(!rect.IsPointInside(mercator::FromLatLon(10000.0 / 111320.0, 0.5)), ());
}

UNIT_TEST(RouteCorridor_FilterResults)
{
  search::RouteCorridor corridor(MakeTestRoute(), 2000.0);

  FrozenDataSource dataSource;
  MwmSet::MwmId const id =
      dataSource.Register(platform::LocalCountryFile::MakeForTesting("minsk-pass")).first;

  search::Results results;
  auto addFeature = [&results, &id](m2::PointD const & pt)
  {
    search::Result res(pt, "test");
    res.FromFeature({id, 0 /* index */}, 0, 0, {});
    results.AddResultNoChecks(std::move(res));
  };

  m2::PointD const nearRoute = mercator::FromLatLon(1000.0 / 111320.0, 0.5);
  m2::PointD const farFromRoute = mercator::FromLatLon(10000.0 / 111320.0, 0.5);
  addFeature(nearRoute);
  addFeature(farFromRoute);

  search::FilterResultsByCorridor(results, corridor);

  TEST_EQUAL(results.GetCount(), 1, ());
  TEST_ALMOST_EQUAL_ULPS(results[0].GetFeatureCenter(), nearRoute, ());
  double const d = results[0].m_details.m_distanceFromRouteMeters;
  TEST_GREATER(d, 0.0, ());
  TEST_LESS_OR_EQUAL(d, 2000.0, ());
}

UNIT_TEST(RouteCorridor_MultiSegmentNearest)
{
  // Regression: the distance must be measured to the NEAREST segment, not the
  // last one. L-shaped route; the point sits near the first segment and far
  // from the last one.
  m2::PointD const a = mercator::FromLatLon(0.0, 0.0);
  m2::PointD const b = mercator::FromLatLon(0.0, 0.5);
  m2::PointD const c = mercator::FromLatLon(1.0, 0.5);
  search::RouteCorridor corridor(m2::PolylineD({a, b, c}), 2000.0);

  // ~500 m north of the first segment's midpoint, ~55 km from the last segment.
  m2::PointD const nearFirst = mercator::FromLatLon(500.0 / 111320.0, 0.25);
  double const d = corridor.GetDistanceFromRouteMeters(nearFirst);
  TEST_GREATER(d, 0.0, ());
  TEST_LESS(d, 2000.0, ());
  TEST(corridor.Contains(nearFirst), ());
}

UNIT_TEST(RouteCorridor_Invalid)
{
  search::RouteCorridor empty;
  TEST(!empty.IsValid(), ());
  TEST(!empty.Contains(m2::PointD::Zero()), ());
}
}  // namespace search_tests
