#include "testing/testing.hpp"

#include "routing/hazard_warnings_manager.hpp"

#include "platform/settings.hpp"

#include "geometry/point2d.hpp"

#include <vector>

using namespace routing;

namespace
{
m2::PointD const kTestPoint(10.0, 10.0);

// SetEnabled persists to the real settings file, which would poison other tests
// and later runs; clear the persisted key before and after each guarded test.
struct EnabledGuard
{
  EnabledGuard() { settings::Delete("hazard_warnings_enabled"); }
  ~EnabledGuard() { settings::Delete("hazard_warnings_enabled"); }
};

struct CallbackRecorder
{
  int m_showCount = 0;
  int m_clearCount = 0;
  m2::PointD m_lastShownPoint;

  HazardWarningsManager::HazardShowCallback ShowCallback()
  {
    return [this](m2::PointD const & point)
    {
      ++m_showCount;
      m_lastShownPoint = point;
    };
  }

  HazardWarningsManager::HazardClearCallback ClearCallback()
  {
    return [this]() { ++m_clearCount; };
  }
};

UNIT_TEST(HazardWarnings_GetStateForDistance)
{
  using State = HazardWarningsManager::State;

  // Far ahead — no warning.
  TEST_EQUAL(HazardWarningsManager::GetStateForDistance(1000.0), State::None, ());
  // Just inside the warning distance.
  TEST_EQUAL(HazardWarningsManager::GetStateForDistance(500.0), State::Warn, ());
  // Approaching.
  TEST_EQUAL(HazardWarningsManager::GetStateForDistance(100.0), State::Warn, ());
  // Just passed but still inside the clear zone.
  TEST_EQUAL(HazardWarningsManager::GetStateForDistance(-49.0), State::Warn, ());
  // Well behind — should be cleared.
  TEST_EQUAL(HazardWarningsManager::GetStateForDistance(-51.0), State::Passed, ());
}

UNIT_TEST(HazardWarnings_WarnOncePerHazard)
{
  EnabledGuard guard;
  CallbackRecorder recorder;
  HazardWarningsManager manager;
  manager.SetShowCallback(recorder.ShowCallback());
  manager.SetClearCallback(recorder.ClearCallback());
  manager.SetHazards({{1000.0, kTestPoint}});

  // Too far — nothing happens.
  manager.ProcessPosition(100.0);
  TEST_EQUAL(recorder.m_showCount, 0, ());
  TEST_EQUAL(recorder.m_clearCount, 0, ());
  TEST_EQUAL(manager.GetAnnouncementId(), 0, ());

  // Inside the warning zone — one show callback and one announcement id bump.
  manager.ProcessPosition(600.0);
  TEST_EQUAL(recorder.m_showCount, 1, ());
  TEST_EQUAL(recorder.m_lastShownPoint, kTestPoint, ());
  TEST_EQUAL(manager.GetAnnouncementId(), 1, ());

  // Further position updates do not re-announce the same hazard.
  manager.ProcessPosition(700.0);
  TEST_EQUAL(recorder.m_showCount, 1, ());
  TEST_EQUAL(manager.GetAnnouncementId(), 1, ());

  // Passing the hazard clears the warning.
  manager.ProcessPosition(1100.0);
  TEST_EQUAL(recorder.m_clearCount, 1, ());
}

UNIT_TEST(HazardWarnings_MultipleHazardsInSequence)
{
  EnabledGuard guard;
  CallbackRecorder recorder;
  HazardWarningsManager manager;
  manager.SetShowCallback(recorder.ShowCallback());
  manager.SetClearCallback(recorder.ClearCallback());
  manager.SetHazards({{1000.0, m2::PointD(1.0, 1.0)}, {2000.0, m2::PointD(2.0, 2.0)}});

  manager.ProcessPosition(600.0);
  TEST_EQUAL(recorder.m_showCount, 1, ());
  TEST_EQUAL(manager.GetAnnouncementId(), 1, ());

  manager.ProcessPosition(1100.0);
  TEST_EQUAL(recorder.m_clearCount, 1, ());

  // The second hazard is announced after the first one is passed.
  manager.ProcessPosition(1600.0);
  TEST_EQUAL(recorder.m_showCount, 2, ());
  TEST_EQUAL(recorder.m_lastShownPoint, m2::PointD(2.0, 2.0), ());
  TEST_EQUAL(manager.GetAnnouncementId(), 2, ());

  manager.ProcessPosition(2100.0);
  TEST_EQUAL(recorder.m_clearCount, 2, ());
}

UNIT_TEST(HazardWarnings_Disabled)
{
  EnabledGuard guard;
  CallbackRecorder recorder;
  HazardWarningsManager manager;
  manager.SetShowCallback(recorder.ShowCallback());
  manager.SetClearCallback(recorder.ClearCallback());
  manager.SetHazards({{1000.0, kTestPoint}});
  manager.SetEnabled(false);

  TEST(!manager.Enabled(), ());
  manager.ProcessPosition(600.0);
  manager.ProcessPosition(1100.0);
  TEST_EQUAL(recorder.m_showCount, 0, ());
  TEST_EQUAL(recorder.m_clearCount, 0, ());
  TEST_EQUAL(manager.GetAnnouncementId(), 0, ());
}

UNIT_TEST(HazardWarnings_Reset)
{
  EnabledGuard guard;
  CallbackRecorder recorder;
  HazardWarningsManager manager;
  manager.SetShowCallback(recorder.ShowCallback());
  manager.SetClearCallback(recorder.ClearCallback());
  manager.SetHazards({{1000.0, kTestPoint}});
  manager.ProcessPosition(600.0);
  TEST_EQUAL(recorder.m_showCount, 1, ());

  manager.Reset();
  TEST_EQUAL(recorder.m_clearCount, 1, ());

  // After reset the hazards are gone, so no new warnings are issued.
  manager.ProcessPosition(700.0);
  TEST_EQUAL(recorder.m_showCount, 1, ());
}

UNIT_TEST(HazardWarnings_UnsortedHazardsAreSorted)
{
  EnabledGuard guard;
  CallbackRecorder recorder;
  HazardWarningsManager manager;
  manager.SetShowCallback(recorder.ShowCallback());
  manager.SetClearCallback(recorder.ClearCallback());
  // Deliberately unsorted: the manager must warn about the closest hazard first.
  manager.SetHazards({{2000.0, m2::PointD(2.0, 2.0)}, {1000.0, m2::PointD(1.0, 1.0)}});

  manager.ProcessPosition(600.0);
  TEST_EQUAL(recorder.m_showCount, 1, ());
  TEST_EQUAL(recorder.m_lastShownPoint, m2::PointD(1.0, 1.0), ());
}
}  // namespace
