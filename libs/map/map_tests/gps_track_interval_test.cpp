#include "testing/testing.hpp"

#include "map/gps_track_filter.hpp"

#include <vector>

namespace gps_track_interval_test
{
using Points = std::vector<location::GpsInfo>;

location::GpsInfo Make(double timestamp, double lat, double lon, double speed)
{
  location::GpsInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = lat;
  info.m_longitude = lon;
  info.m_horizontalAccuracy = 5;
  info.m_source = location::EAndroidNative;
  return info;
}

// Degrees of longitude per meter at the equator.
double constexpr kDegPerMeter = 1.0 / 111320.0;

// Straight line to the east, 15 m every second at 15 m/s: every point passes the accuracy checks.
Points MakeWalk(size_t count)
{
  Points points;
  for (size_t i = 0; i < count; ++i)
    points.push_back(Make(100.0 + i, 0.0, 15.0 * i * kDegPerMeter, 15.0));
  return points;
}

// Restores the previous interval so that other tests in the binary are not affected
// (StoreSamplingInterval also persists to the settings file).
struct IntervalGuard
{
  IntervalGuard() : m_prev(GpsTrackFilter::GetSamplingInterval()) { GpsTrackFilter::StoreSamplingInterval(0.0); }
  ~IntervalGuard() { GpsTrackFilter::StoreSamplingInterval(m_prev); }
  double const m_prev;
};

UNIT_TEST(GpsTrackFilter_ZeroIntervalRecordsEveryFix)
{
  IntervalGuard guard;
  GpsTrackFilter filter;
  Points out;
  filter.Process(MakeWalk(10), out);
  TEST_EQUAL(out.size(), 10, ());
}

UNIT_TEST(GpsTrackFilter_IntervalDecimatesPoints)
{
  IntervalGuard guard;
  // One point every 5 seconds: t = 100, 105, 110, 115, 120, 125.
  GpsTrackFilter::StoreSamplingInterval(5.0);
  GpsTrackFilter filter;
  Points out;
  filter.Process(MakeWalk(21), out);
  TEST_EQUAL(out.size(), 5, ());
  TEST_EQUAL(out[0].m_timestamp, 100.0, ());
  TEST_EQUAL(out[1].m_timestamp, 105.0, ());
  TEST_EQUAL(out[4].m_timestamp, 120.0, ());
}

UNIT_TEST(GpsTrackFilter_IntervalChangeTakesEffectImmediately)
{
  IntervalGuard guard;
  GpsTrackFilter filter;
  Points out;
  filter.Process(MakeWalk(5), out);
  TEST_EQUAL(out.size(), 5, ());

  // A wider interval set after the filter was created still applies to it.
  GpsTrackFilter::StoreSamplingInterval(10.0);
  out.clear();
  filter.Process(MakeWalk(20), out);
  // The first new fix that is at least 10 s after the last accepted one.
  TEST_EQUAL(out.size(), 1, ());
  TEST_EQUAL(out[0].m_timestamp, 114.0, ());
}

UNIT_TEST(GpsTrackFilter_FinalizeAppendsLastFixBeyondInterval)
{
  IntervalGuard guard;
  GpsTrackFilter::StoreSamplingInterval(5.0);
  GpsTrackFilter filter;
  Points out;
  filter.Process(MakeWalk(5), out);
  TEST_EQUAL(out.size(), 1, ());

  // The final fix is force-appended even though it is inside the interval,
  // so the saved GPX track ends where the user actually stopped.
  auto const last = Make(104.0, 0.0, 15.0 * 4 * kDegPerMeter, 0.0);
  out.clear();
  filter.Process({last}, out);
  TEST(out.empty(), ());
  filter.Finalize(out);
  TEST_EQUAL(out.size(), 1, ());
  TEST_EQUAL(out[0].m_timestamp, last.m_timestamp, ());
}
}  // namespace gps_track_interval_test
