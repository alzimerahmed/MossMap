#pragma once

#include "routing/route.hpp"

#include "geometry/point2d.hpp"
#include "platform/location.hpp"

#include "base/thread_checker.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace routing
{
// A hazard is a road danger mapped in OSM (e.g. a node/way tagged hazard=*),
// projected onto the route as a distance from the route start.
struct HazardOnRoute
{
  double m_distFromBeginMeters = -1.0;
  m2::PointD m_position;

  HazardOnRoute() = default;
  HazardOnRoute(double distFromBeginMeters, m2::PointD const & position)
    : m_distFromBeginMeters(distFromBeginMeters)
    , m_position(position)
  {}

  bool IsValid() const { return m_distFromBeginMeters >= 0.0; }
};

// Proximity warning manager for road hazards during navigation. It mirrors the
// SpeedCameraManager pattern but is intentionally simpler: hazards have no speed
// limit, so there is a single warning zone and one voice/visual warning per hazard.
//
// The manager is fed hazard positions with |SetHazards()| (data source: OSM hazard
// features projected onto the route). On every position update it fires the show
// callback when the driver approaches the next hazard and the clear callback after
// passing it. The pending-announcement flag is consumed by the client (UI thread)
// to play the voice warning through its own TTS pipeline.
class HazardWarningsManager
{
public:
  using HazardShowCallback = std::function<void(m2::PointD const &)>;
  using HazardClearCallback = std::function<void()>;

  enum class State
  {
    None,       // No hazard close enough to warn about.
    Warn,       // Hazard ahead within the warning distance.
    Passed      // Hazard has been passed, warning should be cleared.
  };

  HazardWarningsManager();

  // Decision logic for the distance to the closest hazard ahead. Exposed as a pure
  // function for unit testing.
  static State GetStateForDistance(double distToHazardMeters);

  void SetRoute(std::weak_ptr<Route> route) { m_route = std::move(route); }
  void SetHazards(std::vector<HazardOnRoute> hazards);
  void SetShowCallback(HazardShowCallback && callback) { m_showCallback = std::move(callback); }
  void SetClearCallback(HazardClearCallback && callback) { m_clearCallback = std::move(callback); }

  void SetEnabled(bool enabled);
  bool Enabled() const { return m_enabled; }

  void OnLocationPositionChanged(location::GpsInfo const & info);
  // Testable core: the same logic but with the distance already passed on the route.
  void ProcessPosition(double passedDistanceMeters);

  /// \brief Monotonic id of the last issued hazard announcement (0 = none).
  /// Thread-safe: incremented on the location-processing thread; consumers poll
  /// it and announce when it grows past their last-seen value.
  uint32_t GetAnnouncementId() const { return m_announcementId.load(std::memory_order_relaxed); }

  void Reset();

private:
  // Distance ahead of the driver at which the hazard warning is issued.
  // TODO: scale with current speed like the speed-camera manager does; a fixed
  // window gives very different lead times at 30 vs 90 km/h.
  static double constexpr kHazardWarnDistanceM = 500.0;
  // Distance after passing the hazard at which the warning is cleared.
  static double constexpr kHazardClearDistanceM = 50.0;

  std::weak_ptr<Route> m_route;
  // Hazards sorted by |m_distFromBeginMeters|; |m_nextHazardIndex| points to the
  // first hazard that has not been announced yet.
  std::vector<HazardOnRoute> m_hazards;
  size_t m_nextHazardIndex = 0;
  bool m_activeHazard = false;
  bool m_enabled = true;

  mutable std::atomic<uint32_t> m_announcementId{0};

  HazardShowCallback m_showCallback = [](m2::PointD const & /* point */) {};
  HazardClearCallback m_clearCallback = []() {};

  DECLARE_THREAD_CHECKER(m_threadChecker);
};

std::string DebugPrint(HazardWarningsManager::State state);
}  // namespace routing
