#include "routing/hazard_warnings_manager.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <string_view>

namespace routing
{
namespace
{
std::string_view constexpr kHazardWarningsEnabledKey = "hazard_warnings_enabled";
}

HazardWarningsManager::HazardWarningsManager()
{
  bool enabled = true;
  if (settings::Get(kHazardWarningsEnabledKey, enabled))
    m_enabled = enabled;
}

// static
HazardWarningsManager::State HazardWarningsManager::GetStateForDistance(double distToHazardMeters)
{
  if (distToHazardMeters < -kHazardClearDistanceM)
    return State::Passed;

  if (distToHazardMeters <= kHazardWarnDistanceM)
    return State::Warn;

  return State::None;
}

void HazardWarningsManager::SetHazards(std::vector<HazardOnRoute> hazards)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Drop invalid entries so they cannot shadow valid distances when sorting.
  hazards.erase(std::remove_if(hazards.begin(), hazards.end(),
                               [](HazardOnRoute const & h) { return !h.IsValid(); }),
                hazards.end());
  std::sort(hazards.begin(), hazards.end(), [](HazardOnRoute const & lhs, HazardOnRoute const & rhs)
  {
    return lhs.m_distFromBeginMeters < rhs.m_distFromBeginMeters;
  });
  m_hazards = std::move(hazards);
  m_nextHazardIndex = 0;
  if (m_activeHazard)
  {
    m_activeHazard = false;
    m_clearCallback();
  }
}

void HazardWarningsManager::SetEnabled(bool enabled)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_enabled == enabled)
    return;

  m_enabled = enabled;
  settings::Set(kHazardWarningsEnabledKey, enabled);

  if (!enabled && m_activeHazard)
  {
    m_activeHazard = false;
    m_clearCallback();
  }
}

void HazardWarningsManager::OnLocationPositionChanged(location::GpsInfo const & info)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto route = m_route.lock();
  if (!route)
    return;

  ProcessPosition(route->GetCurrentDistanceFromBeginMeters());
}

void HazardWarningsManager::ProcessPosition(double passedDistanceMeters)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!m_enabled)
    return;

  // Clear the active hazard once it is behind us.
  if (m_activeHazard)
  {
    bool activePassed = true;
    if (m_nextHazardIndex > 0)
    {
      auto const & last = m_hazards[m_nextHazardIndex - 1];
      activePassed = last.m_distFromBeginMeters - passedDistanceMeters < -kHazardClearDistanceM;
    }
    if (activePassed)
    {
      m_activeHazard = false;
      m_clearCallback();
    }
  }

  // Announce the next hazard ahead if it is close enough.
  while (m_nextHazardIndex < m_hazards.size())
  {
    auto const & hazard = m_hazards[m_nextHazardIndex];
    double const distToHazard = hazard.m_distFromBeginMeters - passedDistanceMeters;
    State const state = GetStateForDistance(distToHazard);
    if (state == State::None)
      break;

    if (state == State::Passed)
    {
      // Hazard was passed without being announced (e.g. it was set after the fact).
      ++m_nextHazardIndex;
      continue;
    }

    m_activeHazard = true;
    m_announcementId.fetch_add(1, std::memory_order_relaxed);
    m_showCallback(hazard.m_position);
    ++m_nextHazardIndex;
    break;
  }
}

void HazardWarningsManager::Reset()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_hazards.clear();
  m_nextHazardIndex = 0;
  m_activeHazard = false;
  m_clearCallback();
}

std::string DebugPrint(HazardWarningsManager::State state)
{
  switch (state)
  {
  case HazardWarningsManager::State::None: return "None";
  case HazardWarningsManager::State::Warn: return "Warn";
  case HazardWarningsManager::State::Passed: return "Passed";
  }
  UNREACHABLE();
}
}  // namespace routing
