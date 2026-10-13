#include "chamfercartesiansampler.h"

#include "chamferpreparationdiagnostics.h"
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
constexpr double SavedBranchTolerance = 5.0; // Degrees, branch guard at the same saved time only.

struct CartesianTarget
{
  M4d tcp = M4d::Identity();
  std::optional<V6d> referenceJoints; // Present only at a saved animation timestamp.
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

class CartesianSampler
{
public:
  // Saved analytical timing/robot and controller sample clock define preparation.
  CartesianSampler(const ChamferMotion& motion, double period)
    : m_motion(motion), m_period(period), m_solver(motion.robot().model),
      m_tcpToFlange(inverseRigidTransform(motion.robot().flangeToTcp)) {}

  ChamferCartesianSamplingResult run()
  {
    QElapsedTimer timer;
    timer.start();
    auto result = runImpl();
    result.diagnostics = m_solver.diagnostics();
    result.diagnostics.insert("samplingAndValidationMs", timer.nsecsElapsed() / 1e6);
    result.diagnostics.insert("retainedSamples", result.data ? result.data->samples.size() : m_data.samples.size());
    result.diagnostics.insert("intervalValidationSamples", m_intervalValidationSamples);
    return result;
  }

private:
  ChamferCartesianSamplingResult runImpl()
  {
    const QString sourceError = inspectSource();
    if (!sourceError.isEmpty()) return {{}, sourceError};
    double cycles = std::ceil(duration() / m_period);
    if (cycles * m_period < duration()) ++cycles;
    if (cycles > 1.0 && (cycles - 1.0) * m_period >= duration()) --cycles;
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
      const double clockTime = index * m_period;
      const double time = index == static_cast<qsizetype>(cycles) ? duration() : clockTime;
      const V6d& seed = m_data.samples.isEmpty() ? m_data.phasePoses.front().joints
                                               : m_data.samples.back().pose.joints;
      auto current = sampleAt(time, seed);
      if (!current.sample) return {{}, atTime(current.error, time)};
      current.sample->controllerTime = clockTime;
      if (!m_data.samples.isEmpty()) {
        const auto& previous = m_data.samples.back();
        const QString error = inspectInterval(previous, *current.sample);
        if (!error.isEmpty()) return {{}, atTime(error, time)};
        const QString clockError = inspectControllerCycle(previous, *current.sample);
        if (!clockError.isEmpty()) return {{}, atTime(clockError, clockTime)};
        current.sample->machiningSeconds = std::max(0.0,
            std::min(time, m_phaseTimes[2]) - std::max(previous.time, m_phaseTimes[1]));
      } else {
        m_rateSample = *current.sample;
      }
      m_data.samples.append(std::move(*current.sample));
      if (time == duration()) break;
    }
    m_data.phasePoses[2].phase = ChamferMotionPhase::Machining;
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
    if (!m_motion.centralMotion()) return QStringLiteral("Regenerate the path: saved analytical timing is missing.");
    if (!std::isfinite(timing.machiningFeed) || timing.machiningFeed <= 0.0)
      return QStringLiteral("Invalid saved machining speed.");
    if (!std::isfinite(m_period) || m_period <= TimeResolution || points.isEmpty()
        || !m_solver.isValid() || !timing.jointSpeed.allFinite()
        || !timing.jointAcceleration.allFinite() || timing.jointSpeed.minCoeff() <= 0.0
        || timing.jointAcceleration.minCoeff() <= 0.0
        || !std::isfinite(timing.cartesianAcceleration) || timing.cartesianAcceleration <= 0.0)
      return QStringLiteral("Invalid Cartesian sampling clock or robot limits.");
    for (std::size_t joint = 0; joint < DofCount; ++joint) {
      const double ratedSpeed = m_motion.robot().model.joints[joint].ratedPayloadSpeedDegreesPerSecond;
      if (!std::isfinite(ratedSpeed) || ratedSpeed <= 0.0 || timing.jointSpeed[joint] > ratedSpeed)
        return QStringLiteral("Saved joint speed exceeds or lacks a rated-payload model bound.");
    }
    for (std::size_t i = 0; i < boundaries.size(); ++i)
      if (boundaries[i] < 0 || boundaries[i] >= points.size()
          || (i > 0 && boundaries[i] < boundaries[i - 1]))
        return QStringLiteral("Invalid saved phase boundaries.");
    m_start = points[boundaries[2]].time;
    for (std::size_t i = 0; i < m_phaseTimes.size(); ++i) {
      m_phaseTimes[i] = m_motion.centralMotion()->phaseTimes()[i];
      const double saved = points[boundaries[i + 2]].time - m_start;
      if (!std::isfinite(saved) || std::abs(saved - m_phaseTimes[i]) > TimeResolution)
        return QStringLiteral("Saved phase times differ from the analytical trajectory.");
      if (!std::isfinite(m_phaseTimes[i]) || (i > 0 && m_phaseTimes[i] <= m_phaseTimes[i - 1]))
        return QStringLiteral("Invalid saved central timing.");
    }
    if (duration() <= minimumRateStep())
      return QStringLiteral("The saved central duration is below sampling resolution.");
    for (std::size_t phase = 2; phase <= 4; ++phase) {
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

  std::optional<CartesianTarget> targetAt(double time) const
  {
    const auto analytic = m_motion.centralMotion()->evaluate(time);
    if (!analytic) return std::nullopt;
    // A saved configuration witnesses only its own timestamp, not the entire
    // following interval. Sparse animation knots must not cap intervening travel.
    // No interpolation or animation FK determines the Cartesian target.
    const auto& points = m_motion.points();
    qsizetype low = m_motion.boundaries()[2];
    qsizetype high = m_motion.boundaries()[5];
    const double absolute = time == duration() ? points[high].time : m_start + time;
    while (low < high) {
      const qsizetype middle = low + (high - low + 1) / 2;
      if (points[middle].time <= absolute) low = middle;
      else high = middle - 1;
    }
    std::optional<V6d> reference;
    if (std::abs(points[low].time - absolute) <= TimeResolution)
      reference = points[low].joints;
    else if (low < m_motion.boundaries()[5]
             && std::abs(points[low + 1].time - absolute) <= TimeResolution)
      reference = points[low + 1].joints;
    return CartesianTarget{analytic->tcp, reference,
        static_cast<ChamferMotionPhase>(static_cast<int>(analytic->phase) + 2), analytic->progress};
  }

  SampleResult sampleAt(double time, const V6d& seed) const
  {
    const auto target = targetAt(time);
    if (!target)
      return {{}, QStringLiteral("Cannot evaluate the saved analytical Cartesian trajectory.")};
    const auto solution = m_solver.solveIK(target->tcp * m_tcpToFlange, seed);
    if (!solution || !solution->q.allFinite())
      return {{}, QStringLiteral("No continuous IK for the Cartesian sample.")};
    for (std::size_t joint = 0; joint < DofCount; ++joint) {
      const auto& limits = m_motion.robot().model.joints[joint];
      if (solution->q[joint] < limits.qMin || solution->q[joint] > limits.qMax)
        return {{}, QStringLiteral("Cartesian sampling exceeds a joint position limit.")};
    }
    if (target->referenceJoints
        && (solution->q - *target->referenceJoints).cwiseAbs().maxCoeff() > SavedBranchTolerance)
      return {{}, QStringLiteral("Cartesian sampling differs from the saved joint configuration at the same time.")};
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
    const auto& central = *m_motion.centralMotion();
    QVector<double> switches;
    for (double time : central.phaseTimes()) switches.append(time);
    const auto leadIn = central.profile(ChamferPhase::LeadIn);
    const auto leadOut = central.profile(ChamferPhase::LeadOut);
    if (leadIn) switches.append(leadIn->rampSeconds);
    if (leadOut) switches.append(m_phaseTimes[2] + leadOut->cruiseSeconds);
    for (double time : switches)
      if (time > first.time + TimeResolution && time < last.time - TimeResolution)
        times.append(time);
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
    const double allowedFeed = limits.machiningFeed;
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
      ++m_intervalValidationSamples;
      const auto evaluated = sampleAt(time, previous.pose.joints);
      if (time == last.time && evaluated.sample
          && (evaluated.sample->pose.joints - last.pose.joints).cwiseAbs().maxCoeff() > 1e-6)
        return QStringLiteral("Intermediate validation changes the cycle endpoint IK winding.");
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

  QString inspectControllerCycle(const ChamferCartesianSample& first,
                                 const ChamferCartesianSample& last)
  {
    // Every correction occupies one complete controller period, including the
    // final correction when the mathematical trajectory finishes between ticks.
    IntervalRates rates;
    rates.duration = m_period;
    rates.cartesian = (last.pose.tcp.block<3, 1>(0, 3) - first.pose.tcp.block<3, 1>(0, 3)) / m_period;
    rates.joints = (last.pose.joints - first.pose.joints) / m_period;
    const auto& limits = m_motion.robot().timing;
    if (!rates.cartesian.allFinite() || !rates.joints.allFinite()
        || rates.cartesian.norm() > limits.machiningFeed * (1.0 + RateTolerance)
        || (rates.joints.cwiseAbs().array() > limits.jointSpeed.array() * (1.0 + RateTolerance)).any())
      return QStringLiteral("Controller-cycle nominal feed or joint speed exceeds a saved bound.");
    const double elapsed = 0.5 * (m_period + m_previousControllerRates.duration);
    const V3d acceleration = (rates.cartesian - m_previousControllerRates.cartesian) / elapsed;
    const V6d joints = (rates.joints - m_previousControllerRates.joints) / elapsed;
    if (!acceleration.allFinite() || !joints.allFinite()
        || acceleration.norm() > limits.cartesianAcceleration * (1.0 + RateTolerance)
        || (joints.cwiseAbs().array() > limits.jointAcceleration.array() * (1.0 + RateTolerance)).any())
      return QStringLiteral("Controller-cycle nominal acceleration exceeds a simulation bound.");
    if (last.terminal) {
      if (rates.cartesian.norm() / (0.5 * m_period) > limits.cartesianAcceleration * (1.0 + RateTolerance)
          || (rates.joints.cwiseAbs().array() / (0.5 * m_period)
              > limits.jointAcceleration.array() * (1.0 + RateTolerance)).any())
        return QStringLiteral("Final controller-cycle stop exceeds a simulation acceleration bound.");
      const Eigen::Quaterniond start(M3d(first.pose.tcp.block<3, 3>(0, 0)));
      const Eigen::Quaterniond end(M3d(last.pose.tcp.block<3, 3>(0, 0)));
      // Include mathematical completion itself if it occurs between ticks.
      const double finish = (duration() - first.controllerTime) / m_period;
      for (double fraction : {0.25, 0.5, 0.75, std::clamp(finish, 0.0, 1.0)}) {
        const double time = std::min(first.controllerTime + fraction * m_period, duration());
        const auto expected = m_motion.centralMotion()->evaluate(time);
        if (!expected) return QStringLiteral("Cannot evaluate final controller-cycle target.");
        const V3d linear = (1.0 - fraction) * first.pose.tcp.block<3, 1>(0, 3)
            + fraction * last.pose.tcp.block<3, 1>(0, 3);
        const Eigen::Quaterniond rotation(M3d(expected->tcp.block<3, 3>(0, 0)));
        if ((expected->tcp.block<3, 1>(0, 3) - linear).norm() > m_tolerances.positionTolerance
            || start.slerp(fraction, end).angularDistance(rotation)
                > m_tolerances.orientationToleranceDegrees * GeomConst::DegToRad)
          return QStringLiteral("Final full controller cycle exceeds Cartesian interpolation tolerance.");
      }
    }
    m_previousControllerRates = rates;
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
  double m_period;
  ChamferProfiledKinematics m_solver;
  M4d m_tcpToFlange = M4d::Identity();
  ChamferSamplingParameters m_tolerances;
  double m_start = 0.0;
  std::array<double, 4> m_phaseTimes{};
  ChamferCartesianSamplingData m_data;
  ChamferCartesianSample m_rateSample;
  IntervalRates m_previousRates;
  IntervalRates m_previousControllerRates;
  qint64 m_intervalValidationSamples = 0;
};

} // namespace

ChamferCartesianSamplingResult sampleChamferCartesian(const ChamferMotion& motion, double cyclePeriod)
{
  return CartesianSampler(motion, cyclePeriod).run();
}
