#include "search/route_corridor.hpp"

#include "search/geometry_utils.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace search
{
namespace
{
// Squared distance from |pt| to segment [a, b] in mercator; |closest| gets the
// projection of |pt| onto the segment (clamped to the segment's ends).
double SquaredSegmentDistance(m2::PointD const & pt, m2::PointD const & a, m2::PointD const & b,
                              m2::PointD & closest)
{
  m2::PointD const ab = b - a;
  double const lenSq = ab.SquaredLength();
  if (lenSq == 0.0)
  {
    closest = a;
    return pt.SquaredLength(a);
  }

  double const t = std::clamp(m2::DotProduct(pt - a, ab) / lenSq, 0.0, 1.0);
  closest = a + ab * t;
  return pt.SquaredLength(closest);
}
}  // namespace

RouteCorridor::RouteCorridor(m2::PolylineD polyline, double radiusMeters)
  : m_polyline(std::move(polyline))
  , m_radiusMeters(radiusMeters)
{
  CHECK_GREATER(m_polyline.GetSize(), 1, ());
  CHECK_GREATER(m_radiusMeters, 0.0, ());

  m_rect = m_polyline.GetLimitRect();
  // Inflate by the radius in mercator units. Mercator meters-per-unit grows
  // towards the poles, so using the equator factor over-inflates slightly at
  // high latitudes — safe for a search viewport, exactness is not required.
  double const r = mercator::MetersToMercator(m_radiusMeters);
  m_rect.Inflate(r, r);
}

double RouteCorridor::GetDistanceFromRouteMeters(m2::PointD const & pt) const
{
  ASSERT(IsValid(), ());
  auto const & points = m_polyline.GetPoints();
  double minSq = std::numeric_limits<double>::max();
  m2::PointD segClosest;
  m2::PointD bestClosest;
  for (size_t i = 1; i < points.size(); ++i)
  {
    double const sq = SquaredSegmentDistance(pt, points[i - 1], points[i], segClosest);
    if (sq < minSq)
    {
      minSq = sq;
      bestClosest = segClosest;
    }
  }
  return PointDistance(pt, bestClosest);
}

void FilterResultsByCorridor(Results & results, RouteCorridor const & corridor)
{
  if (!corridor.IsValid())
    return;

  // Single-pass compaction: keep-order two-pointer removal instead of
  // per-index erase (O(n^2) memmove of Result objects on heavy filtering).
  size_t out = 0;
  for (size_t i = 0; i < results.GetCount(); ++i)
  {
    Result & r = results[i];
    bool keep = true;
    if (r.GetResultType() == Result::Type::Feature && r.HasPoint() && !r.IsSuggest())
    {
      double dist;
      if (!corridor.GetRect().IsPointInside(r.GetFeatureCenter()))
      {
        // Cheap early-out: outside the corridor's bounding rect the exact
        // distance is irrelevant — the result is dropped either way.
        dist = corridor.GetRadiusMeters() + 1.0;
      }
      else
      {
        dist = corridor.GetDistanceFromRouteMeters(r.GetFeatureCenter());
      }
      r.m_details.m_distanceFromRouteMeters = dist;
      keep = dist <= corridor.GetRadiusMeters();
    }

    if (out != i)
      results[out] = std::move(r);
    if (keep)
      ++out;
    // Dropped results are overwritten by later kept ones; the tail is trimmed below.
  }
  results.TrimTo(out);
}
}  // namespace search
