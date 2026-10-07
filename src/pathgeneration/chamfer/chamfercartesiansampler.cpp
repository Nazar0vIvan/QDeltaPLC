#include "chamfercartesiansampler.h"

#include "3d/robot/kinematics/kr10kinematics.h"
#include "geometry/utils.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

using namespace RoboCrap3D;

constexpr qsizetype MaxSamples = 200000;
constexpr double TimeResolution = 1e-9;
constexpr double ProgressTolerance = 1e-9;
constexpr double RateTolerance = 1e-6;
constexpr double SavedBranchTolerance = 5.0; // Same maximum knot step as MotionCompiler.

struct CurveInterval
{
  ChamferPhase phase = ChamferPhase::LeadIn;
  double from = 0.0;
  double to = 1.0;
};

struct CartesianTarget
{
  M4d tcp = M4d::Identity();
  V6d referenceJoints = V6d::Zero();
  ChamferMotionPhase phase = ChamferMotionPhase::LeadIn;
  double progress = 0.0;
};

struct SampleResult
{
  std::optional<ChamferCartesianSample> sample;
  QString error;
};

struct IntervalRates
{
  V3d cartesian = V3d::Zero();
  V6d joints = V6d::Zero();
  double duration = 0.0;
};

bool timeBeforePoint(double time, const ChamferJointPoint& point)
{
  return time < point.time;
}

double rotationError(const M4d& first, const M4d& last)
{
  const Eigen::Quaterniond a(M3d(first.block<3, 3>(0, 0)));
  const Eigen::Quaterniond b(M3d(last.block<3, 3>(0, 0)));
  return a.angularDistance(b);
}

class CartesianSampler
{
public:
  // Saved timing/robot, analytic curve and sample clock are independent inputs.
  CartesianSampler(const ChamferMotion& motion, const ChamferPath& path, double period)
    : m_motion(motion), m_path(path), m_period(period), m_solver(motion.robot().model),
      m_tcpToFlange(inverseRigidTransform(motion.robot().flangeToTcp)) {}

  ChamferCartesianSamplingResult run()
  {
    const QString sourceError = inspectSource();
    if (!sourceError.isEmpty()) return {{}, sourceError};
    const double cycles = std::ceil(duration() / m_period);
    if (!std::isfinite(cycles) || cycles > MaxSamples - 1)
      return {{}, QStringLiteral("Cartesian sampling exceeds the 200000-sample limit.")};

    for (std::size_t i = 0; i < m_data.phasePoses.size(); ++i) {
      const auto& seed = m_motion.points()[m_motion.boundaries()[i + 2]].joints;
      const auto boundary = sampleAt(m_phaseTimes[i], seed);
      if (!boundary.sample) return {{}, boundary.error};
      m_data.phasePoses[i] = boundary.sample->pose;
    }
    m_data.samples.reserve(static_cast<qsizetype>(cycles) + 1);
    for (qsizetype index = 0; index <= static_cast<qsizetype>(cycles); ++index) {
      double time = std::min(index * m_period, duration());
      if (duration() - time <= TimeResolution) time = duration();
      const V6d& seed = m_data.samples.isEmpty() ? m_data.phasePoses.front().joints
                                               : m_data.samples.back().pose.joints;
      auto current = sampleAt(time, seed);
      if (!current.sample) return {{}, atTime(current.error, time)};
      if (!m_data.samples.isEmpty()) {
        const auto& previous = m_data.samples.back();
        const QString error = inspectInterval(previous, *current.sample);
        if (!error.isEmpty()) return {{}, atTime(error, time)};
        current.sample->machiningSeconds = std::max(0.0,
            std::min(time, m_phaseTimes[2]) - std::max(previous.time, m_phaseTimes[1]));
      } else {
        m_rateSample = *current.sample;
      }
      m_data.samples.append(std::move(*current.sample));
      if (time == duration()) break;
    }
    const QString stopError = inspectStop();
    if (!stopError.isEmpty()) return {{}, stopError};
    if ((m_data.samples.back().pose.joints - m_data.phasePoses.back().joints)
            .cwiseAbs().maxCoeff() > 1e-6)
      return {{}, QStringLiteral("Cartesian endpoint does not retain the saved joint winding.")};
    return {std::move(m_data), {}};
  }

private:
  double duration() const { return m_phaseTimes.back(); }

  QString atTime(const QString& error, double time) const
  {
    return QStringLiteral("%1 (t=%2 s)").arg(error).arg(time, 0, 'f', 6);
  }

  QString inspectSource()
  {
    const auto& points = m_motion.points();
    const auto& boundaries = m_motion.boundaries();
    const auto& timing = m_motion.robot().timing;
    if (!std::isfinite(m_period) || m_period <= TimeResolution || points.isEmpty()
        || !m_solver.isValid() || !timing.jointSpeed.allFinite()
        || !timing.jointAcceleration.allFinite() || timing.jointSpeed.minCoeff() <= 0.0
        || timing.jointAcceleration.minCoeff() <= 0.0
        || !std::isfinite(timing.cartesianAcceleration) || timing.cartesianAcceleration <= 0.0)
      return QStringLiteral("Invalid Cartesian sampling clock or robot limits.");
    for (std::size_t i = 0; i < boundaries.size(); ++i)
      if (boundaries[i] < 0 || boundaries[i] >= points.size()
          || (i > 0 && boundaries[i] < boundaries[i - 1]))
        return QStringLiteral("Invalid saved phase boundaries.");
    m_start = points[boundaries[2]].time;
    for (std::size_t i = 0; i < m_phaseTimes.size(); ++i) {
      m_phaseTimes[i] = points[boundaries[i + 2]].time - m_start;
      if (!std::isfinite(m_phaseTimes[i]) || (i > 0 && m_phaseTimes[i] <= m_phaseTimes[i - 1]))
        return QStringLiteral("Invalid saved central timing.");
    }
    if (duration() <= minimumRateStep())
      return QStringLiteral("The saved central duration is below sampling resolution.");
    for (std::size_t phase = 2; phase <= 4; ++phase) {
      if (!std::isfinite(feed(phase)) || feed(phase) <= 0.0)
        return QStringLiteral("Cartesian feeds must be finite and positive.");
      double progress = 0.0;
      for (qsizetype i = boundaries[phase] + 1; i <= boundaries[phase + 1]; ++i) {
        const auto& point = points[i];
        if (!std::isfinite(point.time) || point.time <= points[i - 1].time
            || !std::isfinite(point.progress) || point.progress <= progress || point.progress > 1.0
            || !point.tcp.allFinite() || !point.joints.allFinite())
          return QStringLiteral("Invalid saved curve parameter or timed point.");
        progress = point.progress;
      }
      if (progress != 1.0) return QStringLiteral("A saved phase is missing its complete endpoint.");
    }
    return {};
  }

  double feed(std::size_t phase) const
  {
    const auto& timing = m_motion.robot().timing;
    if (phase == 2) return timing.leadInFeed;
    if (phase == 3) return timing.machiningFeed;
    return timing.leadOutFeed;
  }

  std::optional<double> distanceSquared(const CurveInterval& interval, double progress,
                                         const V3d& position) const
  {
    // Phase interval, candidate parameter and reference position define projection.
    const auto point = m_path.evaluatePhase(interval.phase, progress);
    if (!point) return std::nullopt;
    const double distance = (point->tcp.block<3, 1>(0, 3) - position).squaredNorm();
    return std::isfinite(distance) ? std::optional<double>{distance} : std::nullopt;
  }

  std::optional<double> projectProgress(const CurveInterval& interval, const V3d& position) const
  {
    // Saved adaptive intervals bound a local curve projection; never search a full turn.
    constexpr double ratio = 0.6180339887498948482;
    double low = interval.from;
    double high = interval.to;
    double a = high - ratio * (high - low);
    double b = low + ratio * (high - low);
    auto da = distanceSquared(interval, a, position);
    auto db = distanceSquared(interval, b, position);
    if (!da || !db) return std::nullopt;
    for (int iteration = 0; iteration < 48; ++iteration) {
      if (*da <= *db) {
        high = b; b = a; db = da;
        a = high - ratio * (high - low);
        da = distanceSquared(interval, a, position);
      } else {
        low = a; a = b; da = db;
        b = low + ratio * (high - low);
        db = distanceSquared(interval, b, position);
      }
      if (!da || !db) return std::nullopt;
    }
    double best = *da <= *db ? a : b;
    double distance = std::min(*da, *db);
    for (double endpoint : {interval.from, interval.to}) {
      const auto endpointDistance = distanceSquared(interval, endpoint, position);
      if (!endpointDistance) return std::nullopt;
      if (*endpointDistance <= distance) { best = endpoint; distance = *endpointDistance; }
    }
    return best;
  }

  std::optional<CartesianTarget> targetAt(double time) const
  {
    if (!std::isfinite(time) || time < 0.0 || time > duration()) return std::nullopt;
    std::size_t phase = 2;
    while (phase < 4 && time >= m_phaseTimes[phase - 1]) ++phase;
    const auto& points = m_motion.points();
    const auto& boundaries = m_motion.boundaries();
    const double absolute = time == duration() ? points[boundaries[5]].time : m_start + time;
    const auto reference = m_motion.evaluate(absolute);
    if (!reference) return std::nullopt;
    const auto kind = static_cast<ChamferPhase>(phase - 2);
    double progress = 0.0;
    if (time == duration()) progress = 1.0;
    else if (time != m_phaseTimes[phase - 2]) {
      qsizetype low = boundaries[phase];
      qsizetype high = boundaries[phase + 1];
      while (low + 1 < high) {
        const auto middle = low + (high - low) / 2;
        if (points[middle].time <= absolute) low = middle;
        else high = middle;
      }
      const double from = low == boundaries[phase] ? 0.0 : points[low].progress;
      if (absolute == points[low].time) progress = from;
      else if (absolute == points[high].time) progress = points[high].progress;
      else {
        const auto projected = projectProgress({kind, from, points[high].progress},
                                                reference->tcp.block<3, 1>(0, 3));
        if (!projected) return std::nullopt;
        progress = *projected;
      }
    }
    const auto analytic = m_path.evaluatePhase(kind, progress);
    if (!analytic || (analytic->tcp.block<3, 1>(0, 3) - reference->tcp.block<3, 1>(0, 3)).norm()
            > m_tolerances.positionTolerance
        || rotationError(analytic->tcp, reference->tcp)
            > m_tolerances.orientationToleranceDegrees * GeomConst::DegToRad)
      return std::nullopt;
    return CartesianTarget{analytic->tcp, reference->joints,
                           static_cast<ChamferMotionPhase>(phase), progress};
  }

  SampleResult sampleAt(double time, const V6d& seed) const
  {
    const auto target = targetAt(time);
    if (!target)
      return {{}, QStringLiteral("Cannot map saved timing onto the analytic curve within geometric tolerances.")};
    const auto solution = m_solver.solveIK(target->tcp * m_tcpToFlange, seed);
    if (!solution || !solution->q.allFinite())
      return {{}, QStringLiteral("No continuous IK for the Cartesian sample.")};
    for (std::size_t joint = 0; joint < DofCount; ++joint) {
      const auto& limits = m_motion.robot().model.joints[joint];
      if (solution->q[joint] < limits.qMin || solution->q[joint] > limits.qMax)
        return {{}, QStringLiteral("Cartesian sampling exceeds a joint position limit.")};
    }
    if ((solution->q - target->referenceJoints).cwiseAbs().maxCoeff() > SavedBranchTolerance)
      return {{}, QStringLiteral("Cartesian sampling leaves the saved joint configuration.")};
    const M4d actual = m_solver.solveFK(solution->q).back() * m_motion.robot().flangeToTcp;
    if (!actual.allFinite()
        || (actual.block<3, 1>(0, 3) - target->tcp.block<3, 1>(0, 3)).norm() > Kr10PosEps
        || (actual.block<3, 3>(0, 0) - target->tcp.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
      return {{}, QStringLiteral("Cartesian sample fails FK validation.")};
    ChamferCartesianSample sample;
    sample.time = time;
    sample.progress = target->progress;
    sample.pose = {solution->q, target->tcp, target->phase};
    sample.terminal = time == duration();
    return {std::move(sample), {}};
  }

  QVector<double> validationTimes(const ChamferCartesianSample& first,
                                   const ChamferCartesianSample& last) const
  {
    QVector<double> times;
    for (double fraction : {0.25, 0.5, 0.75}) {
      const double time = first.time + fraction * (last.time - first.time);
      if (time > first.time + TimeResolution && time < last.time - TimeResolution)
        times.append(time);
    }
    const auto& points = m_motion.points();
    const auto end = points.cbegin() + m_motion.boundaries()[5] + 1;
    auto point = std::upper_bound(points.cbegin() + m_motion.boundaries()[2], end,
                                  m_start + first.time, &timeBeforePoint);
    for (; point != end && point->time < m_start + last.time; ++point) {
      const double time = point->time - m_start;
      if (time > first.time + TimeResolution && time < last.time - TimeResolution)
        times.append(time);
    }
    times.append(last.time);
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());
    return times;
  }

  double minimumRateStep() const { return m_period * 0.025; } // 100 us at the 4 ms clock.

  QString inspectRates(const ChamferCartesianSample& last)
  {
    const auto& first = m_rateSample;
    const double step = last.time - first.time;
    if (!std::isfinite(step) || step <= 0.0) return QStringLiteral("Cartesian sampling lost time resolution.");
    IntervalRates rates;
    rates.duration = step;
    rates.cartesian = (last.pose.tcp.block<3, 1>(0, 3) - first.pose.tcp.block<3, 1>(0, 3)) / step;
    rates.joints = (last.pose.joints - first.pose.joints) / step;
    const auto& limits = m_motion.robot().timing;
    double allowedFeed = 0.0;
    for (std::size_t phase = 0; phase < 3; ++phase)
      if (first.time < m_phaseTimes[phase + 1] && last.time > m_phaseTimes[phase])
        allowedFeed = std::max(allowedFeed, feed(phase + 2));
    if (!rates.cartesian.allFinite() || !rates.joints.allFinite()
        || rates.cartesian.norm() > allowedFeed * (1.0 + RateTolerance)
        || (rates.joints.cwiseAbs().array() > limits.jointSpeed.array() * (1.0 + RateTolerance)).any())
      return QStringLiteral("Cartesian sampling exceeds a saved feed or joint speed limit.");
    const double elapsed = 0.5 * (step + m_previousRates.duration);
    const V3d acceleration = (rates.cartesian - m_previousRates.cartesian) / elapsed;
    const V6d jointAcceleration = (rates.joints - m_previousRates.joints) / elapsed;
    if (!acceleration.allFinite() || !jointAcceleration.allFinite()
        || acceleration.norm() > limits.cartesianAcceleration * (1.0 + RateTolerance)
        || (jointAcceleration.cwiseAbs().array()
            > limits.jointAcceleration.array() * (1.0 + RateTolerance)).any())
      return QStringLiteral("Cartesian sampling exceeds a saved acceleration limit.");
    m_previousRates = rates;
    m_rateSample = last;
    return {};
  }

  QString inspectInterval(const ChamferCartesianSample& first, const ChamferCartesianSample& last)
  {
    const double intervalDuration = last.time - first.time;
    if (!(intervalDuration > 0.0)) return QStringLiteral("Cartesian sampling lost time resolution.");
    const Eigen::Quaterniond start(M3d(first.pose.tcp.block<3, 3>(0, 0)));
    const Eigen::Quaterniond end(M3d(last.pose.tcp.block<3, 3>(0, 0)));
    if (start.angularDistance(end) > 5.0 * GeomConst::DegToRad)
      return QStringLiteral("A Cartesian cycle has excessive orientation change.");
    auto previous = first;
    for (double time : validationTimes(first, last)) {
      if (time != last.time && time - previous.time <= TimeResolution) continue;
      SampleResult evaluated;
      if (time == last.time) evaluated.sample = last;
      else evaluated = sampleAt(time, previous.pose.joints);
      if (!evaluated.sample) return evaluated.error;
      const auto& current = *evaluated.sample;
      if (current.pose.phase < previous.pose.phase
          || (current.pose.phase == previous.pose.phase
              && current.progress + ProgressTolerance < previous.progress))
        return QStringLiteral("The Cartesian time-to-parameter mapping reverses curve progress.");
      const double fraction = (time - first.time) / intervalDuration;
      const V3d linear = (1.0 - fraction) * first.pose.tcp.block<3, 1>(0, 3)
          + fraction * last.pose.tcp.block<3, 1>(0, 3);
      const Eigen::Quaterniond rotation(M3d(current.pose.tcp.block<3, 3>(0, 0)));
      if ((current.pose.tcp.block<3, 1>(0, 3) - linear).norm() > m_tolerances.positionTolerance
          || start.slerp(fraction, end).angularDistance(rotation)
              > m_tolerances.orientationToleranceDegrees * GeomConst::DegToRad)
        return QStringLiteral("The 4 ms Cartesian interpolation exceeds geometric tolerances.");
      // Geometry keeps nearby knots; derivative estimates need a resolved interval.
      // Reserve a resolved interval for the exact terminal stop as well.
      if (current.terminal || (time - m_rateSample.time >= minimumRateStep()
                              && duration() - time >= minimumRateStep())) {
        const QString rateError = inspectRates(current);
        if (!rateError.isEmpty()) return rateError;
      }
      previous = current;
    }
    return {};
  }

  QString inspectStop() const
  {
    const auto& limits = m_motion.robot().timing;
    const double halfStep = 0.5 * m_previousRates.duration;
    if (!(halfStep > 0.0)
        || m_previousRates.cartesian.norm() / halfStep > limits.cartesianAcceleration * (1.0 + RateTolerance)
        || (m_previousRates.joints.cwiseAbs().array() / halfStep
            > limits.jointAcceleration.array() * (1.0 + RateTolerance)).any())
      return QStringLiteral("The Cartesian terminal stop exceeds a saved acceleration limit.");
    return {};
  }

  const ChamferMotion& m_motion;
  const ChamferPath& m_path;
  double m_period;
  Kr10Kinematics m_solver;
  M4d m_tcpToFlange = M4d::Identity();
  ChamferSamplingParameters m_tolerances;
  double m_start = 0.0;
  std::array<double, 4> m_phaseTimes{};
  ChamferCartesianSamplingData m_data;
  ChamferCartesianSample m_rateSample;
  IntervalRates m_previousRates;
};

} // namespace

ChamferCartesianSamplingResult sampleChamferCartesian(const ChamferMotion& motion, double cyclePeriod)
{
  const auto path = ChamferPath::create(motion.parameters());
  if (!path.path) return {{}, path.error};
  return CartesianSampler(motion, *path.path, cyclePeriod).run();
}
