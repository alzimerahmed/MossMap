#pragma once

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/rect2d.hpp"
#include "search/result.hpp"

#include <vector>

namespace search
{
// A corridor around a route polyline: the set of points within |m_radiusMeters|
// of the polyline. Used for "search along route": results are filtered to the
// corridor and annotated with their distance from the route.
class RouteCorridor
{
public:
  static double constexpr kDefaultRadiusMeters = 2000.0;

  RouteCorridor() = default;
  RouteCorridor(m2::PolylineD polyline, double radiusMeters);

  bool IsValid() const { return m_radiusMeters > 0.0 && m_polyline.GetSize() >= 2; }

  double GetRadiusMeters() const { return m_radiusMeters; }

  // Bounding rect of the corridor (polyline bounds inflated by the radius in
  // mercator units). Suitable as a search viewport so the engine covers the
  // whole route area.
  m2::RectD const & GetRect() const { return m_rect; }

  // Distance from |pt| to the route polyline in meters (min over segments).
  double GetDistanceFromRouteMeters(m2::PointD const & pt) const;

  bool Contains(m2::PointD const & pt) const
  {
    return IsValid() && GetDistanceFromRouteMeters(pt) <= m_radiusMeters;
  }

  m2::PolylineD const & GetPolyline() const { return m_polyline; }

private:
  m2::PolylineD m_polyline;
  double m_radiusMeters = 0.0;
  m2::RectD m_rect;
};

// Keeps only the feature results inside |corridor| and annotates each kept
// feature result with its distance from the route
// (Result::Details::m_distanceFromRouteMeters). Suggestions, coordinate
// results and bookmark results are left untouched.
void FilterResultsByCorridor(Results & results, RouteCorridor const & corridor);
}  // namespace search
