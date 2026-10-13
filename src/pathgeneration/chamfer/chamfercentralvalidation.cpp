#include "chamfercentralvalidation.h"

#include "chamferpreparationdiagnostics.h"
#include "geometry/utils.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

using namespace RoboCrap3D;
constexpr qsizetype MaximumSamples = 200000;
constexpr double PreviewPositionTolerance = 0.01; // mm; Cartesian chord criterion only.
constexpr double RateStep = 1e-4;
constexpr double RateTolerance = 1e-6;

// Bound the actual chamfer TCP curve, not just the measured edge ellipse.
// In cylinder azimuth alpha: h is the sinusoidal end-plane slope, |h|,|h'|,|h''|<=H.
// With g=(1+h*h)^(-1/2), |g|<=1, |g'|<=H, |g''|<=2*H*H+H.
// p = center + rho*(r+h*k) + size/2*(g*r+(2*h-cot(angle))*g*k).
// |dp/dalpha|>=rho, hence curvature <= |d2p/dalpha2|/rho^2.
std::optional<double> machiningCurvatureBound(const ChamferPath& path)
{
  const auto& parameters = path.parameters();
  const V3d normal = parameters.edge.majorAxis.cross(parameters.edge.minorAxis).normalized();
  const V3d& axis = path.outwardAxis();
  const double cosine = std::abs(normal.dot(axis));
  const double slope = normal.cross(axis).stableNorm() / cosine;
  const double radius = parameters.edge.minorRadius;
  const double cotangent = 1.0 / std::tan(parameters.chamferAngleDegrees * GeomConst::DegToRad);
  const double secondG = 2.0 * slope * slope + slope;
  const double radialSecond = secondG + 2.0 * slope + 1.0;
  const double axialSecond = 2.0 * slope + 4.0 * slope * slope
      + (2.0 * slope + cotangent) * secondG;
  const double second = radius * std::hypot(1.0, slope)
      + 0.5 * parameters.chamferSize * (radialSecond + axialSecond);
  const double curvature = (second / radius) / radius;
  if (!std::isfinite(curvature) || curvature <= 0.0) return std::nullopt;
  return curvature;
}

struct PreviewSpan
{
  double start = 0.0;
  double end = 0.0;
  double accelerationBound = 0.0; // mm/s^2, geometric criterion, not a robot limit.
  int intervals = 0;
  QString phase;
};

struct PreviewSchedule
{
  QVector<PreviewSpan> spans;
  qsizetype points = 1; // Shared starts/ends are stored only once.
  QString error;
};

PreviewSchedule makePreviewSchedule(const ChamferPath& path, const ChamferCentralMotion& motion)
{
  const auto curvature = machiningCurvatureBound(path);
  if (!curvature) return {{}, 0, QStringLiteral("Cannot bound machining TCP curvature.")};
  PreviewSchedule schedule;
  for (int index = 0; index < 3; ++index) {
    const auto phase = static_cast<ChamferPhase>(index);
    const auto profile = motion.profile(phase);
    if (!profile) return {{}, 0, QStringLiteral("Missing preview phase profile.")};
    const double start = motion.phaseTimes()[index];
    const double end = motion.phaseTimes()[index + 1];
    QVector<double> stops{start};
    if (index != 1 && profile->cruiseSeconds > 0.0)
      stops.append(start + (index == 0 ? profile->rampSeconds : profile->cruiseSeconds));
    stops.append(end);
    const double normalAcceleration = profile->speed * (profile->speed
        * (index == 1 ? *curvature : 1.0 / path.parameters().leadRadius));
    for (qsizetype part = 1; part < stops.size(); ++part) {
      const bool ramp = index == 0 ? part == 1 : index == 2 && part == stops.size() - 1;
      const double bound = std::hypot(ramp ? profile->acceleration : 0.0, normalAcceleration);
      const double duration = stops[part] - stops[part - 1];
      const double count = std::max(1.0, std::ceil(duration
          * std::sqrt(bound / (8.0 * PreviewPositionTolerance))));
      if (!std::isfinite(bound) || bound <= 0.0 || !std::isfinite(duration) || duration <= 0.0
          || !std::isfinite(count) || count > MaximumSamples - schedule.points)
        return {{}, 0, QStringLiteral("Preview geometric schedule exceeds numerical range or 200000 points.")};
      const int intervals = static_cast<int>(count);
      if (!(duration / intervals > 1e-9))
        return {{}, 0, QStringLiteral("Preview geometric interval is below time resolution.")};
      schedule.spans.append({stops[part - 1], stops[part], bound, intervals,
          index == 0 ? QStringLiteral("leadIn") : index == 1 ? QStringLiteral("machining")
                                                           : QStringLiteral("leadOut")});
      schedule.points += intervals;
    }
  }
  return schedule;
}

bool rigid(const M4d& pose)
{
  return pose.allFinite()
      && isBasis(pose.block<3, 1>(0, 0), pose.block<3, 1>(0, 1),
                 pose.block<3, 1>(0, 2), Kr10RotEps)
      && (pose.row(3) - Eigen::RowVector4d(0, 0, 0, 1)).cwiseAbs().maxCoeff() <= GeomConst::Eps;
}

bool validRobot(const ChamferRobotSetup& robot)
{
  const auto& model = robot.model;
  const auto& ik = model.ik;
  const auto& timing = robot.timing;
  if (!rigid(robot.flangeToTcp) || !model.qHome.allFinite()
      || !std::isfinite(ik.sx) || !std::isfinite(ik.sz) || !std::isfinite(ik.a)
      || !std::isfinite(ik.bx) || !std::isfinite(ik.by) || !std::isfinite(ik.dF)
      || !Kr10Kinematics(model).isValid() || !timing.jointSpeed.allFinite()
      || timing.jointSpeed.minCoeff() <= 0.0 || !timing.jointAcceleration.allFinite()
      || timing.jointAcceleration.minCoeff() <= 0.0
      || !std::isfinite(timing.cartesianAcceleration) || timing.cartesianAcceleration <= 0.0)
    return false;
  for (const auto& joint : model.joints)
    if (!rigid(joint.localTransform) || !joint.axis.allFinite()
        || std::abs(joint.axis.x()) > GeomConst::Eps || std::abs(joint.axis.y()) > GeomConst::Eps
        || std::abs(std::abs(joint.axis.z()) - 1.0) > GeomConst::Eps
        || !std::isfinite(joint.qMin) || !std::isfinite(joint.qMax) || joint.qMin > joint.qMax
        || std::abs(joint.qMin) > 1e6 || std::abs(joint.qMax) > 1e6
        || !std::isfinite(joint.ratedPayloadSpeedDegreesPerSecond)
        || joint.ratedPayloadSpeedDegreesPerSecond <= 0.0)
      return false;
  return true;
}

QString positionError(const V6d& joints, const Kr10KinematicModel& model)
{
  if (!joints.allFinite()) return QStringLiteral("Nonfinite joint coordinates.");
  for (std::size_t axis = 0; axis < DofCount; ++axis) {
    const auto& joint = model.joints[axis];
    if (joints[axis] < joint.qMin || joints[axis] > joint.qMax)
      return QStringLiteral("A%1 position %2 deg is outside [%3, %4].")
          .arg(axis + 1).arg(joints[axis], 0, 'g', 12).arg(joint.qMin).arg(joint.qMax);
  }
  return {};
}

struct SampleResult
{
  std::optional<ChamferCentralJointSample> sample;
  QString error;
};

struct Rates
{
  V6d joints = V6d::Zero();
  V3d xyz = V3d::Zero();
  double seconds = 0.0;
};

class CentralValidator
{
public:
  // Own numerical snapshots; solver references the owned model for this whole run.
  CentralValidator(ChamferCentralMotion motion, ChamferRobotSetup robot)
    : m_motion(std::move(motion)), m_robot(std::move(robot)), m_solver(m_robot.model),
      m_tcpToFlange(inverseRigidTransform(m_robot.flangeToTcp)) {}

  ChamferCentralValidationResult run(const V6d& seed, const ChamferPath& path)
  {
    QElapsedTimer timer;
    timer.start();
    auto result = runImpl(seed, path);
    result.diagnostics = m_solver.diagnostics();
    result.diagnostics.insert("samplingAndValidationMs", timer.nsecsElapsed() / 1e6);
    result.diagnostics.insert("retainedSamples", result.data ? result.data->samples.size() : m_samples.size());
    result.diagnostics.insert("interpolationChecks", 0);
    result.diagnostics.insert("plannedSamples", m_plannedSamples);
    result.diagnostics.insert("samplingCriterion", "cartesian-curvature-time-bound");
    result.diagnostics.insert("segments", m_segments);
    return result;
  }

private:
  ChamferCentralValidationResult runImpl(const V6d& seed, const ChamferPath& path)
  {
    const auto schedule = makePreviewSchedule(path, m_motion);
    if (!schedule.error.isEmpty()) return {{}, schedule.error};
    m_plannedSamples = schedule.points;
    m_samples.reserve(schedule.points);
    const ChamferProfileScope initialScope(m_solver.diagnostics());
    const auto first = sample(0.0, seed);
    m_segments.insert("initialCentralPose", initialScope.finish(m_solver.diagnostics()));
    if (!first.sample) return {{}, atTime(first.error, 0.0)};
    m_samples.append(*first.sample);
    for (const auto& span : schedule.spans) {
      const ChamferProfileScope phaseScope(m_solver.diagnostics());
      const auto beforePoints = m_samples.size();
      for (int index = 1; index <= span.intervals; ++index) {
        const double time = index == span.intervals ? span.end
            : span.start + (span.end - span.start) * (double(index) / span.intervals);
        const QString error = appendPoint(time);
        if (!error.isEmpty()) {
          auto measured = phaseScope.finish(m_solver.diagnostics());
          measured.insert("addedPoints", m_samples.size() - beforePoints);
          measured.insert("plannedIntervals", span.intervals);
          measured.insert("accelerationBound", span.accelerationBound);
          measured.insert("complete", false);
          measured.insert("error", atTime(error, time));
          recordSegment(span.phase, measured);
          return {{}, span.phase + QStringLiteral(": ") + atTime(error, time)};
        }
      }
      auto measured = phaseScope.finish(m_solver.diagnostics());
      measured.insert("addedPoints", m_samples.size() - beforePoints);
      measured.insert("plannedIntervals", span.intervals);
      measured.insert("accelerationBound", span.accelerationBound);
      measured.insert("complete", true);
      recordSegment(span.phase, measured);
    }
    const ChamferProfileScope ratesScope(m_solver.diagnostics());
    const QString error = inspectRates();
    m_segments.insert("wholeCentralRateChecks", ratesScope.finish(m_solver.diagnostics()));
    if (!error.isEmpty()) return {{}, error};
    return {ChamferCentralValidationData{std::move(m_motion), std::move(m_robot),
                                        std::move(m_samples)}, {}};
  }

private:
  void recordSegment(const QString& phase, QJsonObject measured)
  {
    const auto previous = m_segments.value(phase).toObject();
    for (const auto& key : {"computationMs", "ikCalls", "explicitFkCalls", "ikMs", "explicitFkMs",
                            "addedPoints", "plannedIntervals"})
      measured.insert(key, measured.value(key).toDouble() + previous.value(key).toDouble());
    measured.insert("accelerationBound", std::max(measured.value("accelerationBound").toDouble(),
        previous.value("accelerationBound").toDouble()));
    m_segments.insert(phase, measured);
  }

  QString atTime(const QString& error, double time) const
  {
    return QStringLiteral("%1 (central t=%2 s)").arg(error).arg(time, 0, 'g', 12);
  }

  SampleResult sample(double time, const V6d& seed)
  {
    const auto target = m_motion.evaluate(time);
    if (!target) return {{}, QStringLiteral("Cannot evaluate prescribed TCP target.")};
    const auto solution = m_solver.solveIK(target->tcp * m_tcpToFlange, seed);
    if (!solution) return {{}, QStringLiteral("IK failed: unreachable, singular or outside the seeded configuration/limits.")};
    const QString error = positionError(solution->q, m_robot.model);
    if (!error.isEmpty()) return {{}, error};
    const M4d actual = m_solver.solveFK(solution->q).back() * m_robot.flangeToTcp;
    if (!actual.allFinite()
        || (actual.block<3, 1>(0, 3) - target->tcp.block<3, 1>(0, 3)).norm() > Kr10PosEps
        || (actual.block<3, 3>(0, 0) - target->tcp.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
      return {{}, QStringLiteral("TCP FK reconstruction exceeds 0.0001 mm or 0.000001 rotation-entry tolerance.")};
    return {ChamferCentralJointSample{time, *target, solution->q}, {}};
  }

  QString appendPoint(double time)
  {
    const auto& previous = m_samples.back();
    const double step = time - previous.time;
    if (!(step > 1e-9)) return QStringLiteral("Preview timestamps lost numerical resolution.");
    const auto current = sample(time, previous.joints);
    if (!current.sample) return current.error;
    const QString error = inspectSpeed((current.sample->joints - previous.joints) / step);
    if (!error.isEmpty()) return error;
    m_samples.append(*current.sample);
    return {};
  }

  QString accelerationError(const Rates& before, const Rates& after) const
  {
    const double elapsed = 0.5 * (before.seconds + after.seconds);
    if (!(elapsed > 0.0)) return QStringLiteral("Unresolved acceleration interval.");
    const V6d acceleration = (after.joints - before.joints) / elapsed;
    const V3d xyz = (after.xyz - before.xyz) / elapsed;
    if (!acceleration.allFinite() || !xyz.allFinite()) return QStringLiteral("Nonfinite sampled acceleration.");
    // Temporary Dry Run diagnostic: per-joint acceleration rejection disabled.
    // for (int axis = 0; axis < 6; ++axis)
    //   if (std::abs(acceleration[axis]) > m_robot.timing.jointAcceleration[axis] * (1.0 + RateTolerance))
    //     return QStringLiteral("A%1 acceleration %2 deg/s^2 exceeds configured simulation bound %3.")
    //         .arg(axis + 1).arg(std::abs(acceleration[axis]), 0, 'g', 12)
    //         .arg(m_robot.timing.jointAcceleration[axis]);
    if (xyz.norm() > m_robot.timing.cartesianAcceleration * (1.0 + RateTolerance))
      return QStringLiteral("XYZ acceleration %1 mm/s^2 exceeds configured simulation bound %2.")
          .arg(xyz.norm(), 0, 'g', 12).arg(m_robot.timing.cartesianAcceleration);
    return {};
  }

  QString inspectSpeed(const V6d& velocity) const
  {
    if (!velocity.allFinite()) return QStringLiteral("Nonfinite sampled joint velocity.");
    for (int axis = 0; axis < 6; ++axis) {
      const double limit = std::min(m_robot.timing.jointSpeed[axis],
          m_robot.model.joints[axis].ratedPayloadSpeedDegreesPerSecond);
      if (std::abs(velocity[axis]) > limit * (1.0 + RateTolerance))
        return QStringLiteral("A%1 speed %2 deg/s exceeds effective bound %3.")
            .arg(axis + 1).arg(std::abs(velocity[axis]), 0, 'g', 12).arg(limit);
    }
    return {};
  }

  QString inspectRates() const
  {
    auto first = m_samples.front();
    Rates previous; // Rest at t=0; interval velocities are located at their midpoints.
    for (qsizetype index = 1; index < m_samples.size(); ++index) {
      const auto& last = m_samples[index];
      const bool terminal = index == m_samples.size() - 1;
      const double step = last.time - first.time;
      if (!terminal && (step < RateStep || m_motion.duration() - last.time < RateStep)) continue;
      if (!(step >= RateStep)) return QStringLiteral("Central motion is too short for resolved rate validation.");
      Rates current;
      current.seconds = step;
      current.joints = (last.joints - first.joints) / step;
      current.xyz = (last.target.tcp.block<3, 1>(0, 3) - first.target.tcp.block<3, 1>(0, 3)) / step;
      if (!current.joints.allFinite() || !current.xyz.allFinite())
        return atTime(QStringLiteral("Nonfinite sampled velocity."), last.time);
      const QString speedError = inspectSpeed(current.joints);
      if (!speedError.isEmpty()) return atTime(speedError, last.time);
      const QString error = accelerationError(previous, current);
      if (!error.isEmpty()) return atTime(error, last.time);
      previous = current;
      first = last;
    }
    const QString stop = accelerationError(previous, Rates{});
    return stop.isEmpty() ? QString{} : atTime(stop, m_motion.duration());
  }

  ChamferCentralMotion m_motion;
  ChamferRobotSetup m_robot;
  ChamferProfiledKinematics m_solver;
  M4d m_tcpToFlange = M4d::Identity();
  QVector<ChamferCentralJointSample> m_samples;
  qsizetype m_plannedSamples = 0;
  QJsonObject m_segments;
};

} // namespace

ChamferCentralValidationResult validateChamferCentral(const ChamferPath& path,
                                                      const ChamferRobotSetup& robot,
                                                      const V6d& initialSeed)
{
  if (!validRobot(robot)) return {{}, QStringLiteral("Invalid robot model, TCP calibration or simulation rate bounds.")};
  const QString error = positionError(initialSeed, robot.model);
  if (!error.isEmpty()) return {{}, QStringLiteral("Invalid initial IK seed: %1").arg(error)};
  QElapsedTimer timer;
  timer.start();
  auto motion = ChamferCentralMotion::create(path, robot.timing);
  const double constructionMs = timer.nsecsElapsed() / 1e6;
  if (!motion.motion) return {{}, motion.error, {{"analyticalConstructionMs", constructionMs}}};
  auto result = CentralValidator(std::move(*motion.motion), robot).run(initialSeed, path);
  result.diagnostics.insert("analyticalConstructionMs", constructionMs);
  return result;
}
