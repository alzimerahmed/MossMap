#include "map/gps_track_filter.hpp"

#include "coding/point_coding.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <atomic>

namespace
{
// Keys are shared with the Android settings UI through platform/settings.hpp.
std::string_view constexpr kMinHorizontalAccuracyKey = settings::kTrackRecordingMinAccuracy;
std::string_view constexpr kSamplingIntervalKey = settings::kTrackRecordingInterval;

// Minimal horizontal accuracy is required to skip 'bad' points.
// Use 250 meters to allow points from a pure GPS + GPS through wifi.
double constexpr kMinHorizontalAccuracyMeters = 250;

// Default minimal interval between recorded points, seconds.
// 1 second matches the typical rate of GPS fixes and the upstream behavior.
double constexpr kDefaultSamplingIntervalSeconds = 1.0;

// Required for points decimation to reduce number of close points.
double constexpr kClosePointDistanceMeters = 10;

// Max acceptable acceleration to filter gps jumps
double constexpr kMaxAcceptableAcceleration = 2;  // m / sec ^ 2

// Direction-prediction check only applies to gaps shorter than this; across
// longer gaps the direction prediction is meaningless.
double constexpr kMaxGapForDirectionCheck = 10;  // seconds

double constexpr kCosine45degrees = 0.70710678118;

// Process-wide current values, shared by all filter instances so that a change
// in the settings UI takes effect immediately, including during a recording.
std::atomic<double> g_minAccuracy{kMinHorizontalAccuracyMeters};
std::atomic<double> g_minSamplingIntervalSeconds{kDefaultSamplingIntervalSeconds};

// Same tolerance as the KML/GPX import, which drops such points from imported tracks.
bool IsSamePoint(location::GpsInfo const & a, location::GpsInfo const & b)
{
  return AlmostEqualAbs(mercator::FromLatLon(a.m_latitude, a.m_longitude),
                        mercator::FromLatLon(b.m_latitude, b.m_longitude), kMwmPointAccuracy);
}

m2::PointD GetDirection(location::GpsInfo const & from, location::GpsInfo const & to)
{
  m2::PointD const pt0 = mercator::FromLatLon(from.m_latitude, from.m_longitude);
  m2::PointD const pt1 = mercator::FromLatLon(to.m_latitude, to.m_longitude);
  m2::PointD const d = pt1 - pt0;
  return d.IsAlmostZero() ? m2::PointD::Zero() : d.Normalize();
}

double GetDistance(location::GpsInfo const & from, location::GpsInfo const & to)
{
  return ms::DistanceOnEarth(from.m_latitude, from.m_longitude, to.m_latitude, to.m_longitude);
}
}  // namespace

void GpsTrackNullFilter::Process(std::vector<location::GpsInfo> const & inPoints,
                                 std::vector<location::GpsInfo> & outPoints)
{
  outPoints.insert(outPoints.end(), inPoints.begin(), inPoints.end());
}

void GpsTrackFilter::StoreMinHorizontalAccuracy(double value)
{
  // !(value >= 0) also rejects NaN.
  if (!(value >= 0))
  {
    LOG(LWARNING, ("Ignoring invalid track recording accuracy", value));
    return;
  }
  settings::Set(kMinHorizontalAccuracyKey, value);
  g_minAccuracy.store(value, std::memory_order_relaxed);
}

void GpsTrackFilter::StoreSamplingInterval(double seconds)
{
  // !(seconds >= 0) also rejects NaN.
  if (!(seconds >= 0))
  {
    LOG(LWARNING, ("Ignoring invalid track recording interval", seconds));
    return;
  }
  settings::Set(kSamplingIntervalKey, seconds);
  g_minSamplingIntervalSeconds.store(seconds, std::memory_order_relaxed);
}

double GpsTrackFilter::GetMinHorizontalAccuracy()
{
  return g_minAccuracy.load(std::memory_order_relaxed);
}

double GpsTrackFilter::GetSamplingInterval()
{
  return g_minSamplingIntervalSeconds.load(std::memory_order_relaxed);
}

GpsTrackFilter::GpsTrackFilter()
  : m_minAccuracy(kMinHorizontalAccuracyMeters)
  , m_minSamplingInterval(kDefaultSamplingIntervalSeconds)
  , m_countLastInfo(0)
  , m_countAcceptedInfo(0)
{
  settings::TryGet(kMinHorizontalAccuracyKey, m_minAccuracy);
  settings::TryGet(kSamplingIntervalKey, m_minSamplingInterval);
  // !(v >= 0) also rejects NaN read from a corrupted settings file.
  if (!(m_minAccuracy >= 0))
    m_minAccuracy = kMinHorizontalAccuracyMeters;
  if (!(m_minSamplingInterval >= 0))
    m_minSamplingInterval = kDefaultSamplingIntervalSeconds;
  g_minAccuracy.store(m_minAccuracy, std::memory_order_relaxed);
  g_minSamplingIntervalSeconds.store(m_minSamplingInterval, std::memory_order_relaxed);
}

void GpsTrackFilter::Reset()
{
  m_countLastInfo = 0;
  m_countAcceptedInfo = 0;
}

void GpsTrackFilter::Process(GpsVectorT const & inPoints, GpsVectorT & outPoints)
{
  outPoints.reserve(inPoints.size());

  // Pick up settings changes made after this filter was created.
  m_minAccuracy = g_minAccuracy.load(std::memory_order_relaxed);
  m_minSamplingInterval = g_minSamplingIntervalSeconds.load(std::memory_order_relaxed);

  if (m_minAccuracy == 0)
  {
    // Debugging trick to turn off filtering
    outPoints.insert(outPoints.end(), inPoints.begin(), inPoints.end());
    return;
  }

  for (location::GpsInfo const & currInfo : inPoints)
  {
    // Do not accept points from the predictor
    if (currInfo.m_source == location::EPredictor)
      continue;

    // Skip any function without speed
    if (!currInfo.HasSpeed())
      continue;

    // Decimate by time: skip fixes that arrive sooner than the configured interval
    // after the last accepted fix. The last fix is still force-appended in Finalize().
    bool const intervalElapsed =
        m_minSamplingInterval == 0 || m_countAcceptedInfo == 0 ||
        currInfo.m_timestamp - GetLastAcceptedInfo().m_timestamp >= m_minSamplingInterval;
    if (!intervalElapsed)
    {
      AddLastInfo(currInfo);
      continue;
    }

    // When the gap between accepted fixes is long, the direction-prediction
    // check is meaningless (the fix may be a legitimate turn after a pause),
    // so it only applies to short gaps. The gap is only read when a fix has
    // already been accepted (GetLastAcceptedInfo asserts otherwise).
    if (m_countAcceptedInfo == 0 ||
        (IsGoodPoint(currInfo) &&
         (currInfo.m_timestamp - GetLastAcceptedInfo().m_timestamp > kMaxGapForDirectionCheck ||
          m_countLastInfo < 2 || IsGoodVector(currInfo))))
    {
      outPoints.emplace_back(currInfo);
      AddLastAcceptedInfo(currInfo);
    }

    AddLastInfo(currInfo);
  }
}

void GpsTrackFilter::Finalize(GpsVectorT & outPoints)
{
  if (m_countLastInfo == 0)
    return;

  // Force append the last point, if wasn't added before. A repeated fix would only make a zero-length
  // segment in the saved track, which the track geometry does not accept.
  auto const & info = GetLastInfo();
  if (m_countAcceptedInfo == 0)
  {
    // Nothing was accepted (e.g. every fix was decimated or inaccurate);
    // keep the last fix so the track is not silently empty.
    outPoints.push_back(info);
    AddLastAcceptedInfo(info);
    return;
  }

  auto const & lastAccepted = GetLastAcceptedInfo();
  if (info.m_timestamp > lastAccepted.m_timestamp && !IsSamePoint(lastAccepted, info))
  {
    outPoints.push_back(info);
    AddLastAcceptedInfo(info);
  }
}

bool GpsTrackFilter::IsGoodVector(location::GpsInfo const & info) const
{
  ASSERT_GREATER(m_countLastInfo, 1, ());

  double const predictionDistance = GetDistance(m_lastInfo[1], m_lastInfo[0]);  // meters
  double const realDistance = GetDistance(m_lastInfo[0], info);                 // meters

  m2::PointD const predictionDirection = GetDirection(m_lastInfo[1], m_lastInfo[0]);
  m2::PointD const realDirection = GetDirection(m_lastInfo[0], info);

  // Cosine of angle between prediction direction and real direction is
  double const cosine = m2::DotProduct(predictionDirection, realDirection);

  // Acceptable angle must be from 0 to 45 or from 0 to -45.
  // Acceptable distance must be not great than 2x than predicted, otherwise it is jump.
  return (cosine >= kCosine45degrees && realDistance <= std::max(kClosePointDistanceMeters, 2. * predictionDistance));
}

bool GpsTrackFilter::IsGoodPoint(location::GpsInfo const & info) const
{
  // Filter by point accuracy
  if (info.m_horizontalAccuracy > m_minAccuracy)
    return false;

  auto const & lastInfo = GetLastInfo();
  auto const & lastAcceptedInfo = GetLastAcceptedInfo();

  // Time spend to move from the last point to the current point, sec:
  double const timeFromLast = info.m_timestamp - lastInfo.m_timestamp;
  if (timeFromLast <= 0.0)
    return false;

  // Distance in meters between last accepted and current point is, meters:
  double const distanceFromLastAccepted = GetDistance(lastAcceptedInfo, info);

  // Filter point by close distance
  if (distanceFromLastAccepted < kClosePointDistanceMeters)
    return false;

  // Filter point if accuracy areas are intersected
  if (distanceFromLastAccepted < lastAcceptedInfo.m_horizontalAccuracy &&
      info.m_horizontalAccuracy > 0.5 * lastAcceptedInfo.m_horizontalAccuracy)
    return false;

  // Distance in meters between last and current point is, meters:
  double const distanceFromLast = GetDistance(lastInfo, info);

  // Speed to move from the last point to the current point
  double const speedFromLast = distanceFromLast / timeFromLast;

  // Filter by acceleration: skip point if it jumps too far in short time
  double const accelerationFromLast = (speedFromLast - lastInfo.m_speed) / timeFromLast;
  if (accelerationFromLast > kMaxAcceptableAcceleration)
    return false;

  return true;
}

void GpsTrackFilter::AddLastInfo(location::GpsInfo const & info)
{
  m_lastInfo[1] = m_lastInfo[0];
  m_lastInfo[0] = info;
  ++m_countLastInfo;
}

location::GpsInfo const & GpsTrackFilter::GetLastInfo() const
{
  static location::GpsInfo const kEmpty;
  return m_countLastInfo > 0 ? m_lastInfo[0] : kEmpty;
}

location::GpsInfo const & GpsTrackFilter::GetLastAcceptedInfo() const
{
  static location::GpsInfo const kEmpty;
  return m_countAcceptedInfo > 0 ? m_lastAcceptedInfo[0] : kEmpty;
}

void GpsTrackFilter::AddLastAcceptedInfo(location::GpsInfo const & info)
{
  m_lastAcceptedInfo[1] = m_lastAcceptedInfo[0];
  m_lastAcceptedInfo[0] = info;
  ++m_countAcceptedInfo;
}
