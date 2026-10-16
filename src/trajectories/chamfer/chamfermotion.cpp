#include "trajectories/chamfermotion.h"
#include "chamfercentralvalidation.h"

#include "chamferpreparationdiagnostics.h"
#include "geometry/utils.h"

#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {

using namespace RoboCrap3D;
constexpr double TransferPositionTolerance = 0.01; // mm; prescribed TCP chord criterion only.

bool rigid(const M4d& transform)
{
  return transform.allFinite()
      && isBasis(transform.block<3, 1>(0, 0), transform.block<3, 1>(0, 1),
                 transform.block<3, 1>(0, 2), Kr10RotEps)
      && (transform.row(3) - Eigen::RowVector4d(0.0, 0.0, 0.0, 1.0)).cwiseAbs().maxCoeff()
          <= GeomConst::Eps;
}

bool withinLimits(const V6d& joints, const Kr10KinematicModel& model)
{
  if (!joints.allFinite()) return false;
  if (IgnorePreviewJointPositionLimits) return true;
  for (std::size_t i = 0; i < DofCount; ++i)
    if (joints[i] < model.joints[i].qMin || joints[i] > model.joints[i].qMax) return false;
  return true;
}

// Coefficients of a quadratic derivative; negative entries mean no interior root.
std::array<double, 2> quadraticUnitRoots(double quadratic, double linear, double constant)
{
  std::array<double, 2> roots{-1.0, -1.0};
  const double scale = std::max({std::abs(quadratic), std::abs(linear), std::abs(constant)});
  if (scale == 0.0) return roots;
  quadratic /= scale;
  linear /= scale;
  constant /= scale;
  if (quadratic == 0.0) {
    if (linear != 0.0) roots[0] = -constant / linear;
    return roots;
  }
  const double discriminant = std::fma(-4.0 * quadratic, constant, linear * linear);
  if (discriminant < 0.0) return roots;
  // Avoid subtracting nearly equal values when one root is small.
  const double numerator = -0.5 * (linear + std::copysign(std::sqrt(discriminant), linear));
  if (numerator == 0.0) roots[0] = -linear / (2.0 * quadratic);
  else {
    roots[0] = numerator / quadratic;
    roots[1] = constant / numerator;
  }
  return roots;
}

// Polynomial, physical interval duration and robot bounds define an interval audit.
QString jointSplineIntervalError(const std::array<V6d, 4>& coefficients, double seconds,
                                 const ChamferRobotSetup& robot)
{
  for (const auto& coefficient : coefficients)
    if (!coefficient.allFinite()) return QStringLiteral("Nonfinite joint spline coefficient.");
  for (std::size_t axis = 0; axis < DofCount; ++axis) {
    const double a = coefficients[0][axis];
    const double b = coefficients[1][axis];
    const double c = coefficients[2][axis];
    const double d = coefficients[3][axis];
    if (!std::isfinite(6.0 * d) || !std::isfinite(2.0 * c))
      return QStringLiteral("Nonfinite joint spline derivative.");
    const auto roots = quadraticUnitRoots(3.0 * d, 2.0 * c, b);
    const double speedExtremum = d == 0.0 ? -1.0 : -(c / d) / 3.0;
    const std::array<double, 5> candidates{0.0, 1.0, roots[0], roots[1], speedExtremum};
    const auto& joint = robot.model.joints[axis];
    const double speedBound = std::min(robot.timing.jointSpeed[axis],
                                      joint.ratedPayloadSpeedDegreesPerSecond);
    for (double u : candidates) {
      if (!(u >= 0.0 && u <= 1.0)) continue;
      const double angle = ((d * u + c) * u + b) * u + a;
      const double speed = ((3.0 * d * u + 2.0 * c) * u + b) / seconds;
      const double acceleration = ((6.0 * d * u + 2.0 * c) / seconds) / seconds;
      if (!std::isfinite(angle) || !std::isfinite(speed) || !std::isfinite(acceleration))
        return QStringLiteral("A%1 joint spline has a nonfinite angle or derivative.").arg(axis + 1);
      if (!IgnorePreviewJointPositionLimits && (angle < joint.qMin || angle > joint.qMax))
        return QStringLiteral("A%1 joint spline position %2 deg is outside [%3, %4] (interval fraction %5).")
            .arg(axis + 1).arg(angle, 0, 'g', 12).arg(joint.qMin).arg(joint.qMax).arg(u, 0, 'g', 12);
      if (std::abs(speed) > speedBound * (1.0 + 1e-6))
        return QStringLiteral("A%1 joint spline speed %2 deg/s exceeds bound %3 (interval fraction %4).")
            .arg(axis + 1).arg(std::abs(speed), 0, 'g', 12).arg(speedBound).arg(u, 0, 'g', 12);
      // Preserve the temporary preview joint-acceleration magnitude bypass.
    }
  }
  return {};
}

bool validRobot(const ChamferRobotSetup& robot)
{
  const auto& model = robot.model;
  if (!rigid(robot.flangeToTcp) || !model.qHome.allFinite()) return false;
  const auto& ik = model.ik;
  if (!std::isfinite(ik.sx) || !std::isfinite(ik.sz) || !std::isfinite(ik.a)
      || !std::isfinite(ik.bx) || !std::isfinite(ik.by) || !std::isfinite(ik.dF)) return false;
  for (const auto& joint : model.joints) {
    // The existing analytic solver divides by axis.z and resolves integer turns.
    if (!rigid(joint.localTransform) || !joint.axis.allFinite()
        || std::abs(std::abs(joint.axis.z()) - 1.0) > GeomConst::Eps
        || std::abs(joint.axis.x()) > GeomConst::Eps || std::abs(joint.axis.y()) > GeomConst::Eps
        || !std::isfinite(joint.qMin) || !std::isfinite(joint.qMax)
        || joint.qMin > joint.qMax || std::abs(joint.qMin) > 1e6 || std::abs(joint.qMax) > 1e6
        || !std::isfinite(joint.ratedPayloadSpeedDegreesPerSecond)
        || joint.ratedPayloadSpeedDegreesPerSecond <= 0.0)
      return false;
  }
  return withinLimits(model.qHome, model) && Kr10Kinematics(model).isValid();
}

struct TransferProfile
{
  M4d first = M4d::Identity();
  M4d last = M4d::Identity();
  Eigen::Quaterniond firstRotation = Eigen::Quaterniond::Identity();
  Eigen::Quaterniond lastRotation = Eigen::Quaterniond::Identity();
  double length = 0.0;
  double peak = 0.0;
  double ramp = 0.0;
  double cruise = 0.0;

  double duration() const { return 2.0 * ramp + cruise; }
  double progress(double seconds) const
  {
    if (seconds <= 0.0) return 0.0;
    if (seconds >= duration()) return 1.0;
    const double rampDistance = 0.5 * peak * ramp;
    if (seconds < ramp) {
      const double fraction = seconds / ramp;
      return rampDistance * fraction * fraction / length;
    }
    if (seconds < ramp + cruise)
      return (rampDistance + peak * (seconds - ramp)) / length;
    const double remaining = (duration() - seconds) / ramp;
    return std::clamp(1.0 - rampDistance * remaining * remaining / length, 0.0, 1.0);
  }

  M4d target(double seconds) const
  {
    const double u = progress(seconds);
    if (u == 0.0) return first;
    if (u == 1.0) return last;
    M4d pose = first;
    pose.block<3, 1>(0, 3) = (1.0 - u) * first.block<3, 1>(0, 3) + u * last.block<3, 1>(0, 3);
    // Preview rotation follows traveled distance, including both translational ramps.
    pose.block<3, 3>(0, 0) = firstRotation.slerp(u, lastRotation).normalized().toRotationMatrix();
    return pose;
  }
};

// Curve, transfer side and physical speed/acceleration define a straight transfer.
std::optional<TransferProfile> transferProfile(const ChamferPath& path, ChamferPhase side,
                                              const ChamferTimingParameters& timing)
{
  const bool entering = side == ChamferPhase::LeadIn;
  const auto staging = path.stagingPose(side);
  const auto endpoint = path.evaluatePhase(side, entering ? 0.0 : 1.0);
  if (!staging || !endpoint) return std::nullopt;
  TransferProfile result;
  result.first = entering ? *staging : endpoint->tcp;
  result.last = entering ? endpoint->tcp : *staging;
  if (!rigid(result.first) || !rigid(result.last)) return std::nullopt;
  result.firstRotation = Eigen::Quaterniond(M3d(result.first.block<3, 3>(0, 0))).normalized();
  result.lastRotation = Eigen::Quaterniond(M3d(result.last.block<3, 3>(0, 0))).normalized();
  result.length = (result.last.block<3, 1>(0, 3) - result.first.block<3, 1>(0, 3)).stableNorm();
  if (!std::isfinite(result.length) || result.length <= GeomConst::Eps) return std::nullopt;
  // Compare requested ramp distance before multiplying acceleration by length.
  const double requestedRamp = timing.transferFeed / timing.transferAcceleration;
  const bool triangular = !std::isfinite(requestedRamp)
      || timing.transferFeed * requestedRamp >= result.length;
  result.peak = triangular ? std::sqrt(result.length) * std::sqrt(timing.transferAcceleration)
                           : timing.transferFeed;
  result.ramp = result.peak / timing.transferAcceleration;
  result.cruise = triangular ? 0.0 : (result.length - result.peak * result.ramp) / result.peak;
  if (!std::isfinite(result.peak) || result.peak <= 0.0 || !std::isfinite(result.ramp)
      || result.ramp <= 1e-9 || !std::isfinite(result.cruise) || result.cruise < 0.0
      || !std::isfinite(result.duration()) || result.duration() <= result.ramp
      || (result.cruise > 0.0 && result.ramp + result.cruise <= result.ramp))
    return std::nullopt;
  return result;
}

QString centralPhaseName(ChamferMotionPhase phase)
{
  switch (phase) {
  case ChamferMotionPhase::TransferIn: return QStringLiteral("P_s to lead-in");
  case ChamferMotionPhase::LeadIn: return QStringLiteral("lead-in");
  case ChamferMotionPhase::Machining: return QStringLiteral("machining");
  case ChamferMotionPhase::LeadOut: return QStringLiteral("lead-out");
  case ChamferMotionPhase::TransferOut: return QStringLiteral("lead-out to P_s");
  default: return QStringLiteral("unknown phase");
  }
}

struct CompiledPoints
{
  QVector<ChamferJointPoint> points;
  std::array<qsizetype, 8> boundaries{};
  QString error;
  std::optional<ChamferCentralMotion> central;
};

class MotionCompiler
{
public:
  MotionCompiler(const ChamferPath& path, const ChamferRobotSetup& robot)
    : m_path(path), m_robot(robot), m_solver(robot.model), m_tcpToFlange(inverseRigidTransform(robot.flangeToTcp)),
      m_central(path, robot.timing) {}

  QVector<Kr10Kinematics::IkSolution> starts()
  {
    m_starts.clear();
    const auto target = m_path.stagingPose(ChamferPhase::LeadIn);
    if (!target) return {};
    // Probe valid arm/wrist seeds deterministically; the existing solver owns
    // branch selection. HOME is tried first, then interior limit fractions.
    appendStart(m_robot.model.qHome, *target);
    for (double shoulder : {0.15, 0.5, 0.85})
      for (double elbow : {0.15, 0.5, 0.85})
        for (double wrist : {0.25, 0.75}) {
          V6d seed = m_robot.model.qHome;
          seed[1] = seedAngle(1, shoulder);
          seed[2] = seedAngle(2, elbow);
          seed[4] = seedAngle(4, wrist);
          appendStart(seed, *target);
        }
    // A valid full revolution may need a different initial wrist winding.
    // Ask the solver to resolve those turns within limits, rather than wrapping
    // an already compiled path when a wrist reaches its travel limit.
    const QVector<Kr10Kinematics::IkSolution> branches = m_starts;
    for (const auto& branch : branches)
      for (double fourth : {0.0, 1.0})
        for (double sixth : {0.0, 1.0}) {
          V6d seed = branch.q;
          seed[3] = seedAngle(3, fourth);
          seed[5] = seedAngle(5, sixth);
          appendStart(seed, *target);
        }
    return m_starts;
  }

  CompiledPoints compile(const Kr10Kinematics::IkSolution& start)
  {
    const qint64 sharedBefore = m_central.computationNanoseconds();
    QElapsedTimer timer;
    timer.start();
    m_attempt = {};
    QJsonArray angles;
    for (int joint = 0; joint < 6; ++joint) angles.append(start.q[joint]);
    m_attempt.insert("psJointAnglesDegrees", angles);
    auto result = compileImpl(start);
    const qint64 elapsed = timer.nsecsElapsed() - (m_central.computationNanoseconds() - sharedBefore);
    m_attempt.insert("totalMs", std::max(qint64{0}, elapsed) / 1e6);
    m_attempt.insert("error", result.error);
    m_attempt.insert("assembledPoints", result.points.size());
    m_attempts.append(m_attempt);
    return result;
  }

  QJsonObject diagnostics() const
  {
    auto result = m_solver.diagnostics();
    result.insert("attempts", m_attempts);
    result.insert("centralPreparation", m_central.diagnostics());
    return result;
  }

private:
  CompiledPoints compileImpl(const Kr10Kinematics::IkSolution& start)
  {
    m_output = {};
    const ChamferProfileScope homeScope(m_solver.diagnostics());
    m_output.points.append(forward(m_robot.model.qHome, 0.0));
    m_attempt.insert("initialHomePose", homeScope.finish(m_solver.diagnostics()));
    m_phase = ChamferMotionPhase::Approach;
    if (!appendPtp(start.q)) return std::move(m_output);
    m_previous = start;
    m_output.boundaries[1] = m_output.points.size() - 1;
    m_phase = ChamferMotionPhase::TransferIn;
    m_output.error = appendTransfer(ChamferPhase::LeadIn);
    if (!m_output.error.isEmpty()) return std::move(m_output);
    m_output.boundaries[2] = m_output.points.size() - 1;
    auto validated = m_central.validate(m_robot, m_previous);
    m_attempt.insert("central", validated.diagnostics);
    if (!validated.data) { m_output.error = validated.error; return std::move(m_output); }
    const auto& central = m_central.motion();
    const auto& samples = validated.data->samples;
    if (samples.isEmpty() || (samples.front().solution.q - m_output.points.back().joints)
            .cwiseAbs().maxCoeff() > 1e-6) {
      m_output.error = QStringLiteral("Lead-in does not retain the transfer's joint configuration.");
      return std::move(m_output);
    }
    QElapsedTimer assemblyTimer;
    assemblyTimer.start();
    const double offset = m_output.points.back().time;
    std::size_t phase = 0;
    for (qsizetype i = 1; i < samples.size(); ++i) {
      if (m_output.points.size() >= 400000) {
        m_output.error = QStringLiteral("Complete motion exceeds the 400000-point limit.");
        return std::move(m_output);
      }
      const auto& sample = samples[i];
      ChamferJointPoint point{sample.solution.q, sample.actualTcp, sample.target.progress};
      point.time = offset + sample.time;
      if (phase >= 3) {
        m_output.error = QStringLiteral("Unexpected sample after the central endpoint.");
        return std::move(m_output);
      }
      const bool boundary = sample.time == central.phaseTimes()[phase + 1];
      if (boundary) point.progress = 1.0; // Shared point belongs to the ending phase.
      m_output.points.append(point);
      if (boundary) {
        m_output.boundaries[phase + 3] = m_output.points.size() - 1;
        ++phase;
      }
    }
    if (phase != 3) {
      m_output.error = QStringLiteral("Validated central motion is missing a phase endpoint.");
      return std::move(m_output);
    }
    m_attempt.insert("centralAssemblyMs", assemblyTimer.nsecsElapsed() / 1e6);
    m_previous = samples.back().solution;
    m_output.central = central; // Keep the shared preparation for any later candidate.
    m_phase = ChamferMotionPhase::TransferOut;
    m_output.error = appendTransfer(ChamferPhase::LeadOut);
    if (!m_output.error.isEmpty()) return std::move(m_output);
    m_output.boundaries[6] = m_output.points.size() - 1;
    m_phase = ChamferMotionPhase::ReturnHome;
    if (!appendPtp(m_robot.model.qHome)) return std::move(m_output);
    m_output.boundaries[7] = m_output.points.size() - 1;
    const ChamferProfileScope finalScope(m_solver.diagnostics());
    for (const auto& point : m_output.points)
      if (!rigid(point.tcp) || !withinLimits(point.joints, m_robot.model)) {
        m_output.error = QStringLiteral("Invalid FK pose or joint position in the assembled motion.");
        break;
      }
    m_attempt.insert("wholeMotionChecks", finalScope.finish(m_solver.diagnostics()));
    return std::move(m_output);
  }

private:
  double seedAngle(std::size_t index, double fraction) const
  {
    const auto& joint = m_robot.model.joints[index];
    return (1.0 - fraction) * joint.qMin + fraction * joint.qMax;
  }

  std::optional<std::pair<Kr10Kinematics::IkSolution, M4d>> inverse(
      const M4d& tcp, const Kr10Kinematics::IkSolution& seed) const
  {
    const M4d flange = tcp * m_tcpToFlange;
    const auto solution = m_solver.calculateIK(flange, seed);
    if (!solution || !withinLimits(solution->q, m_robot.model)) return std::nullopt;
    const auto transforms = m_solver.solveFK(solution->q);
    if (!Kr10Kinematics::matchesFlange(flange, transforms.back()))
      return std::nullopt;
    return std::pair{m_solver.configuration(solution->q, transforms),
                     M4d(transforms.back() * m_robot.flangeToTcp)};
  }

  void appendStart(const V6d& seed, const M4d& target)
  {
    const auto previous = m_solver.configuration(seed, m_solver.solveFK(seed));
    const auto solution = inverse(target, previous);
    if (!solution) return;
    for (const auto& existing : m_starts)
      if ((existing.q - solution->first.q).cwiseAbs().maxCoeff() < 1e-6) return;
    m_starts.append(solution->first);
  }

  ChamferJointPoint forward(const V6d& joints, double progress) const
  {
    return {joints, m_solver.solveFK(joints).back() * m_robot.flangeToTcp, progress};
  }

  bool appendPtp(const V6d& end)
  {
    const ChamferProfileScope scope(m_solver.diagnostics());
    const auto before = m_output.points.size();
    const bool success = appendPtpImpl(end);
    auto measured = scope.finish(m_solver.diagnostics());
    measured.insert("addedPoints", m_output.points.size() - before);
    measured.insert("complete", success);
    m_attempt.insert(m_phase == ChamferMotionPhase::Approach ? "homeToPs" : "psToHome", measured);
    return success;
  }

  bool appendPtpImpl(const V6d& end)
  {
    const V6d start = m_output.points.back().joints;
    const int intervals = std::max(1, static_cast<int>(std::ceil((end - start).cwiseAbs().maxCoeff() / 2.0)));
    if (intervals > 400000 - m_output.points.size()) {
      m_output.error = QStringLiteral("Complete motion exceeds the 400000-point limit.");
      return false;
    }
    for (int i = 1; i <= intervals; ++i) {
      const double progress = static_cast<double>(i) / intervals;
      const V6d joints = i == intervals ? end : V6d(start + progress * (end - start));
      m_output.points.append(forward(joints, progress));
    }
    return true;
  }

  QString appendTransfer(ChamferPhase side)
  {
    QElapsedTimer timer;
    timer.start();
    const auto before = m_output.points.size();
    const ChamferProfileScope scope(m_solver.diagnostics());
    const QString error = appendTransferImpl(side);
    auto measured = scope.finish(m_solver.diagnostics());
    measured.insert("addedPoints", m_output.points.size() - before);
    measured.insert("schedule", m_transferSchedule);
    measured.insert("complete", error.isEmpty());
    measured.insert("samplingCriterion", "cartesian-acceleration-time-bound");
    measured.insert("positionToleranceMm", TransferPositionTolerance);
    m_attempt.insert(side == ChamferPhase::LeadIn ? "psToLeadIn" : "leadOutToPs", measured);
    const QString prefix = side == ChamferPhase::LeadIn ? "transferIn" : "transferOut";
    m_attempt.insert(prefix + "Ms", timer.nsecsElapsed() / 1e6);
    m_attempt.insert(prefix + "Points", m_output.points.size() - before);
    return error;
  }

  QString appendTransferImpl(ChamferPhase side)
  {
    m_transferSchedule = {};
    const auto profile = transferProfile(m_path, side, m_robot.timing);
    if (!profile) return QStringLiteral("Invalid or degenerate staging transfer profile.");
    const qsizetype first = m_output.points.size() - 1;
    const M4d& actualStart = m_output.points.back().tcp;
    if ((actualStart.block<3, 1>(0, 3) - profile->first.block<3, 1>(0, 3)).norm() > Kr10PosEps
        || (actualStart.block<3, 3>(0, 0) - profile->first.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
      return QStringLiteral("Staging transfer does not match its preceding TCP endpoint.");
    const double offset = m_output.points.back().time;
    const std::array<double, 4> stops{0.0, profile->ramp,
                                    profile->ramp + profile->cruise, profile->duration()};
    std::array<int, 3> intervals{};
    qsizetype plannedPoints = 1;
    // Count all points before IK. Straight TCP motion has zero normal acceleration.
    for (std::size_t stage = 0; stage < intervals.size(); ++stage) {
      const double duration = stops[stage + 1] - stops[stage];
      if (duration == 0.0) continue;
      const double acceleration = stage == 1 ? 0.0 : m_robot.timing.transferAcceleration;
      const double count = std::max(1.0, std::ceil(duration * std::sqrt(acceleration / (8.0 * TransferPositionTolerance))));
      if (!std::isfinite(duration) || duration <= 0.0 || !std::isfinite(count)
          || count > 200000 - plannedPoints || !(duration / count > 1e-9))
        return QStringLiteral("Transfer schedule exceeds numerical range or 200000 points.");
      intervals[stage] = static_cast<int>(count);
      plannedPoints += intervals[stage];
      m_transferSchedule.append(QJsonObject{{"startSeconds", stops[stage]},
          {"endSeconds", stops[stage + 1]}, {"intervals", intervals[stage]},
          {"accelerationBoundMmPerSecondSquared", acceleration}});
    }
    if (plannedPoints - 1 > 400000 - m_output.points.size())
      return QStringLiteral("Complete motion exceeds the 400000-point limit.");
    for (std::size_t stage = 0; stage < intervals.size(); ++stage) {
      for (int index = 1; index <= intervals[stage]; ++index) {
        const double local = index == intervals[stage] ? stops[stage + 1]
            : stops[stage] + (stops[stage + 1] - stops[stage])
                * (static_cast<double>(index) / intervals[stage]);
        const double time = offset + local;
        if (!std::isfinite(time) || !(time > m_output.points.back().time))
          return QStringLiteral("Transfer timestamps cannot be resolved.");
        const M4d target = profile->target(local);
        const auto solution = inverse(target, m_previous);
        if (!solution)
          return QStringLiteral("%1: IK, joint position or TCP reconstruction failed at %2 s.")
              .arg(centralPhaseName(m_phase)).arg(local, 0, 'g', 12);
        ChamferJointPoint point{solution->first.q, solution->second, profile->progress(local)};
        if ((point.tcp.block<3, 1>(0, 3) - target.block<3, 1>(0, 3)).norm() > Kr10PosEps
            || (point.tcp.block<3, 3>(0, 0) - target.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
          return QStringLiteral("%1: TCP fails FK reconstruction tolerance at %2 s.")
              .arg(centralPhaseName(m_phase)).arg(local, 0, 'g', 12);
        point.time = time;
        m_output.points.append(point);
        m_previous = solution->first;
      }
    }
    return inspectTransferRates(first);
  }

  QString inspectTransferRates(qsizetype first) const
  {
    const auto& points = m_output.points;
    qsizetype anchor = first;
    V6d previous = V6d::Zero();
    double previousStep = 0.0;
    for (qsizetype i = first + 1; i < points.size(); ++i) {
      const double dt = points[i].time - points[i - 1].time;
      const V6d velocity = (points[i].joints - points[i - 1].joints) / dt;
      if (!velocity.allFinite()) return QStringLiteral("Nonfinite transfer velocity.");
      for (int axis = 0; axis < 6; ++axis)
        if (std::abs(velocity[axis]) > m_robot.timing.jointSpeed[axis] * (1.0 + 1e-6))
          return QStringLiteral("%1: A%2 speed exceeds %3 deg/s at %4 s.")
              .arg(centralPhaseName(m_phase)).arg(axis + 1)
              .arg(m_robot.timing.jointSpeed[axis]).arg(points[i].time);
      const double step = points[i].time - points[anchor].time;
      if (i + 1 < points.size() && (step < 1e-4 || points.back().time - points[i].time < 1e-4)) continue;
      if (step < 1e-4) return QStringLiteral("Transfer is too short for resolved rate validation.");
      const V6d current = (points[i].joints - points[anchor].joints) / step;
      const V6d acceleration = (current - previous) / (0.5 * (previousStep + step));
      if (!acceleration.allFinite()) return QStringLiteral("Nonfinite transfer acceleration.");
      // Temporary Dry Run diagnostic: per-joint acceleration rejection disabled.
      // for (int axis = 0; axis < 6; ++axis)
      //   if (std::abs(acceleration[axis]) > m_robot.timing.jointAcceleration[axis] * (1.0 + 1e-6))
      //     return QStringLiteral("%1: A%2 acceleration exceeds simulation bound %3 deg/s^2 at %4 s.")
      //         .arg(centralPhaseName(m_phase)).arg(axis + 1)
      //         .arg(m_robot.timing.jointAcceleration[axis]).arg(points[i].time);
      previous = current;
      previousStep = step;
      anchor = i;
    }
    // Temporary Dry Run diagnostic: per-joint terminal-stop acceleration rejection disabled.
    // for (int axis = 0; axis < 6; ++axis)
    //   if (std::abs(previous[axis]) / (0.5 * previousStep)
    //       > m_robot.timing.jointAcceleration[axis] * (1.0 + 1e-6))
    //     return QStringLiteral("%1: A%2 terminal stop exceeds its simulation acceleration bound.")
    //         .arg(centralPhaseName(m_phase)).arg(axis + 1);
    return {};
  }

  const ChamferPath& m_path;
  const ChamferRobotSetup& m_robot;
  ChamferProfiledKinematics m_solver;
  M4d m_tcpToFlange = M4d::Identity();
  ChamferCentralPreparation m_central;
  ChamferMotionPhase m_phase = ChamferMotionPhase::TransferIn;
  CompiledPoints m_output;
  QVector<Kr10Kinematics::IkSolution> m_starts;
  Kr10Kinematics::IkSolution m_previous;
  QJsonObject m_attempt;
  QJsonArray m_attempts;
  QJsonArray m_transferSchedule;
};

double smoothProgress(double u)
{
  return u * u * u * (10.0 + u * (-15.0 + 6.0 * u));
}

double smoothVelocity(double u)
{
  return 30.0 * u * u * (1.0 - u) * (1.0 - u);
}

double inverseProgress(double progress)
{
  if (progress <= 0.0) return 0.0;
  if (progress >= 1.0) return 1.0;
  double low = 0.0;
  double high = 1.0;
  for (int i = 0; i < 50; ++i) {
    const double middle = 0.5 * (low + high);
    if (smoothProgress(middle) < progress) low = middle;
    else high = middle;
  }
  return 0.5 * (low + high);
}

bool validTiming(const ChamferTimingParameters& timing)
{
  return std::isfinite(timing.machiningFeed) && timing.machiningFeed > 0.0
      && std::isfinite(timing.accelerationDistanceFraction) && timing.accelerationDistanceFraction > 0.0
      && timing.accelerationDistanceFraction <= 1.0
      && std::isfinite(timing.transferFeed) && timing.transferFeed > 0.0
      && std::isfinite(timing.transferAcceleration) && timing.transferAcceleration > 0.0
      && std::isfinite(timing.cartesianAcceleration) && timing.cartesianAcceleration > 0.0
      && timing.jointSpeed.allFinite() && timing.jointSpeed.minCoeff() > 0.0
      && timing.jointAcceleration.allFinite() && timing.jointAcceleration.minCoeff() > 0.0
      && std::isfinite(timing.auxiliaryScale) && timing.auxiliaryScale > 0.0
      && timing.auxiliaryScale <= 1.0;
}

class HomeTimer
{
public:
  HomeTimer(CompiledPoints data, const ChamferRobotSetup& robot)
    : m_data(std::move(data)), m_robot(robot) {}

  CompiledPoints run()
  {
    timePtp(0);
    const double offset = m_data.points[m_data.boundaries[1]].time;
    for (qsizetype i = m_data.boundaries[1] + 1; i <= m_data.boundaries[6]; ++i)
      m_data.points[i].time += offset;
    timePtp(6);
    for (qsizetype i = 1; i < m_data.points.size(); ++i)
      if (!std::isfinite(m_data.points[i].time)
          || m_data.points[i].time <= m_data.points[i - 1].time) {
        m_data.error = QStringLiteral("Motion timing exceeds numerical resolution.");
        break;
      }
    return std::move(m_data);
  }

private:
  void timePtp(std::size_t phase)
  {
    const auto first = m_data.boundaries[phase];
    const auto last = m_data.boundaries[phase + 1];
    const V6d delta = m_data.points[last].joints - m_data.points[first].joints;
    const auto& settings = m_robot.timing;
    // Quintic synchronized progress: max s'=1.875, max |s''|=10/sqrt(3).
    double duration = 0.001; // A coincident HOME endpoint retains a finite phase.
    for (int joint = 0; joint < 6; ++joint) {
      duration = std::max(duration, 1.875 * std::abs(delta[joint])
          / (settings.jointSpeed[joint] * settings.auxiliaryScale));
      duration = std::max(duration, std::sqrt((10.0 / std::sqrt(3.0)) * std::abs(delta[joint])
          / (settings.jointAcceleration[joint] * settings.auxiliaryScale)));
    }
    const double startTime = m_data.points[first].time;
    for (qsizetype i = first + 1; i <= last; ++i) {
      const double u = inverseProgress(m_data.points[i].progress);
      m_data.points[i].time = startTime + duration * u;
      const V6d velocity = delta * (smoothVelocity(u) / duration);
      if (!velocity.allFinite())
        m_data.error = QStringLiteral("Motion timing exceeds numerical resolution.");
    }
  }

  CompiledPoints m_data;
  const ChamferRobotSetup& m_robot;
};

} // namespace

ChamferMotion::ChamferMotion(const ChamferPath& path, const ChamferRobotSetup& robot,
                             QVector<ChamferJointPoint> points, const std::array<qsizetype, 8>& boundaries)
  : m_parameters(path.parameters()), m_robot(robot), m_points(std::move(points)), m_boundaries(boundaries) {}

void ChamferMotion::setCentralMotion(ChamferCentralMotion motion)
{
  m_centralMotion = std::move(motion);
}

QString ChamferMotion::preparePlaybackSplines()
{
  m_jointSpline.resize(m_boundaries[6] - m_boundaries[1]);
  const std::array<std::array<std::size_t, 2>, 3> ranges{{{1, 2}, {2, 5}, {5, 6}}};
  const std::array<QString, 3> names{QStringLiteral("P_s to lead-in"),
      QStringLiteral("Lead-in / machining / lead-out"), QStringLiteral("lead-out to P_s")};
  for (std::size_t index = 0; index < ranges.size(); ++index) {
    const auto& range = ranges[index];
    const QString error = prepareJointSpline(m_boundaries[range[0]], m_boundaries[range[1]]);
    if (!error.isEmpty()) return QStringLiteral("%1: %2").arg(names[index], error);
  }
  return {};
}

QString ChamferMotion::prepareJointSpline(qsizetype first, qsizetype last)
{
  if (first < 0 || last >= m_points.size() || last <= first)
    return QStringLiteral("Missing joint spline points.");
  const qsizetype count = last - first + 1;
  QVector<double> steps(count - 1);
  QVector<V6d> slopes(count - 1, V6d::Zero());
  for (qsizetype i = 0; i + 1 < count; ++i) {
    steps[i] = m_points[first + i + 1].time - m_points[first + i].time;
    if (!std::isfinite(steps[i]) || steps[i] <= 1e-9)
      return QStringLiteral("Joint spline timestamps cannot be resolved.");
    slopes[i] = (m_points[first + i + 1].joints - m_points[first + i].joints) / steps[i];
    if (!slopes[i].allFinite()) return QStringLiteral("Nonfinite joint spline secant speed.");
  }
  // Row-scaled clamped spline system for second derivatives (deg/s^2).
  // Endpoint slopes are zero. The same unknown is shared across phase boundaries.
  QVector<double> upper(count, 0.0);
  QVector<V6d> second(count, V6d::Zero());
  upper[0] = 0.5;
  second[0] = 3.0 * slopes[0] / steps[0];
  if (!second[0].allFinite()) return QStringLiteral("Nonfinite joint spline boundary equation.");
  for (qsizetype i = 1; i < count; ++i) {
    double lower = 1.0;
    double next = 0.0;
    V6d rhs = V6d::Zero();
    if (i + 1 == count) rhs = -6.0 * slopes[i - 1] / steps[i - 1];
    else {
      const double sum = steps[i - 1] + steps[i];
      if (!std::isfinite(sum)) return QStringLiteral("Nonfinite joint spline interval sum.");
      lower = steps[i - 1] / sum;
      next = steps[i] / sum;
      rhs = 6.0 * (slopes[i] - slopes[i - 1]) / sum;
    }
    const double diagonal = 2.0 - lower * upper[i - 1];
    if (!std::isfinite(diagonal) || diagonal <= 0.0 || !rhs.allFinite())
      return QStringLiteral("Invalid joint spline equation.");
    upper[i] = next / diagonal;
    second[i] = (rhs - lower * second[i - 1]) / diagonal;
    if (!second[i].allFinite()) return QStringLiteral("Nonfinite joint spline solution.");
  }
  for (qsizetype i = count - 1; i > 0; --i) {
    second[i - 1] -= upper[i - 1] * second[i];
    if (!second[i - 1].allFinite()) return QStringLiteral("Nonfinite joint spline back substitution.");
  }
  for (qsizetype i = 0; i + 1 < count; ++i) {
    const double square = steps[i] * steps[i];
    const V6d delta = m_points[first + i + 1].joints - m_points[first + i].joints;
    const std::array<V6d, 4> coefficients{
        m_points[first + i].joints,
        V6d(delta - (square / 6.0) * (2.0 * second[i] + second[i + 1])),
        V6d((square / 2.0) * second[i]),
        V6d((square / 6.0) * (second[i + 1] - second[i]))};
    const QString error = jointSplineIntervalError(coefficients, steps[i], m_robot);
    if (!error.isEmpty())
      return QStringLiteral("%1 (motion interval [%2, %3] s).")
          .arg(error).arg(m_points[first + i].time, 0, 'g', 12)
          .arg(m_points[first + i + 1].time, 0, 'g', 12);
    m_jointSpline[first - m_boundaries[1] + i] = coefficients;
  }
  return {};
}

ChamferMotionResult ChamferMotion::create(const ChamferPath& path, const ChamferRobotSetup& robot)
{
  QElapsedTimer totalTimer;
  totalTimer.start();
  if (!validRobot(robot)) return {{}, QStringLiteral("Invalid robot model, rated joint speeds, HOME limits or flange-to-TCP transform.")};
  ChamferRobotSetup boundedRobot = robot;
  // The model is authoritative, including regeneration of paths saved with the old cap.
  for (std::size_t joint = 0; joint < DofCount; ++joint)
    boundedRobot.timing.jointSpeed[joint] = robot.model.joints[joint].ratedPayloadSpeedDegreesPerSecond;
  if (!validTiming(boundedRobot.timing)) return {{}, QStringLiteral("Feeds, transfer acceleration and simulation limits must be finite and positive; acceleration distance and auxiliary scale must be in (0, 1].")};
  MotionCompiler compiler(path, boundedRobot);
  QElapsedTimer searchTimer;
  searchTimer.start();
  const auto starts = compiler.starts();
  const double searchMs = searchTimer.nsecsElapsed() / 1e6;
  QJsonArray splineAttempts;
  QString error = QStringLiteral("No IK solution for the approach P_s pose in the searched configurations.");
  for (const auto& start : starts) {
    auto compiled = compiler.compile(start);
    if (!compiled.error.isEmpty()) { error = compiled.error; continue; }
    QElapsedTimer homeTiming;
    homeTiming.start();
    auto timed = HomeTimer(std::move(compiled), boundedRobot).run();
    const double homeTimingMs = homeTiming.nsecsElapsed() / 1e6;
    if (!timed.error.isEmpty()) { error = timed.error; continue; }
    ChamferMotion motion(path, boundedRobot, std::move(timed.points), timed.boundaries);
    if (!timed.central) return {{}, QStringLiteral("Missing prescribed central trajectory.")};
    motion.setCentralMotion(std::move(*timed.central));
    QElapsedTimer splineTimer;
    splineTimer.start();
    const QString splineError = motion.preparePlaybackSplines();
    QJsonObject splineDiagnostic;
    splineDiagnostic.insert("computationMs", splineTimer.nsecsElapsed() / 1e6);
    splineDiagnostic.insert("centralIntervals", timed.boundaries[5] - timed.boundaries[2]);
    splineDiagnostic.insert("transferInIntervals", timed.boundaries[2] - timed.boundaries[1]);
    splineDiagnostic.insert("transferOutIntervals", timed.boundaries[6] - timed.boundaries[5]);
    splineDiagnostic.insert("attemptIndex", compiler.diagnostics().value("attempts").toArray().size() - 1);
    splineDiagnostic.insert("error", splineError);
    splineAttempts.append(splineDiagnostic);
    if (!splineError.isEmpty()) { error = splineError; continue; }
    auto diagnostics = compiler.diagnostics();
    diagnostics.insert("jointSplineAttempts", splineAttempts);
    diagnostics.insert("homeTimingAndTimestampChecksMs", homeTimingMs);
    diagnostics.insert("totalMs", totalTimer.nsecsElapsed() / 1e6);
    diagnostics.insert("configurationSearchMs", searchMs);
    diagnostics.insert("candidateCount", starts.size());
    diagnostics.insert("retainedPoints", motion.points().size());
    QJsonArray phasePoints;
    for (std::size_t phase = 0; phase < 7; ++phase)
      phasePoints.append(motion.boundaries()[phase + 1] - motion.boundaries()[phase]);
    diagnostics.insert("phaseIntervals", phasePoints);
    return {std::move(motion), {}, std::move(diagnostics)};
  }
  auto diagnostics = compiler.diagnostics();
  diagnostics.insert("jointSplineAttempts", splineAttempts);
  diagnostics.insert("totalMs", totalTimer.nsecsElapsed() / 1e6);
  diagnostics.insert("configurationSearchMs", searchMs);
  diagnostics.insert("candidateCount", starts.size());
  return {{}, error, std::move(diagnostics)};
}

std::optional<std::pair<V6d, ChamferMotionPhase>> ChamferMotion::evaluateJoints(double seconds) const
{
  if (!std::isfinite(seconds) || m_points.isEmpty()) return std::nullopt;
  const double time = std::clamp(seconds, 0.0, duration());
  if (time == 0.0) return std::pair{m_points.front().joints, ChamferMotionPhase::Approach};
  if (time == duration()) return std::pair{m_points.back().joints, ChamferMotionPhase::ReturnHome};
  std::size_t phase = 0;
  while (phase < 6 && time >= m_points[m_boundaries[phase + 1]].time) ++phase;
  const auto first = m_boundaries[phase];
  const auto last = m_boundaries[phase + 1];
  V6d joints = V6d::Zero();
  if (phase == 0 || phase == 6) {
    const double u = (time - m_points[first].time) / (m_points[last].time - m_points[first].time);
    joints = m_points[first].joints + smoothProgress(u) * (m_points[last].joints - m_points[first].joints);
  } else {
    qsizetype low = first;
    qsizetype high = last;
    while (low + 1 < high) {
      const auto middle = low + (high - low) / 2;
      if (m_points[middle].time <= time) low = middle;
      else high = middle;
    }
    const auto& a = m_points[low];
    const auto& b = m_points[high];
    const double fraction = (time - a.time) / (b.time - a.time);
    const qsizetype interval = low - m_boundaries[1];
    if (interval < 0 || interval >= m_jointSpline.size()) return std::nullopt;
    const auto& coefficients = m_jointSpline[interval];
    // Preserve exact IK values at saved knots, including full-turn phase boundaries.
    if (time == a.time) joints = a.joints;
    else if (time == b.time) joints = b.joints;
    else joints = ((coefficients[3] * fraction + coefficients[2]) * fraction
                   + coefficients[1]) * fraction + coefficients[0];
  }
  if (!withinLimits(joints, m_robot.model)) return std::nullopt;
  return std::pair{joints, static_cast<ChamferMotionPhase>(phase)};
}

std::optional<ChamferPlaybackPose> ChamferMotion::evaluate(double seconds) const
{
  const auto pose = evaluateJoints(seconds);
  if (!pose) return std::nullopt;
  const auto& [joints, phase] = *pose;
  if (seconds <= 0.0) return ChamferPlaybackPose{joints, m_points.front().tcp, phase};
  if (seconds >= duration()) return ChamferPlaybackPose{joints, m_points.back().tcp, phase};
  const M4d tcp = Kr10Kinematics(m_robot.model).solveFK(joints).back() * m_robot.flangeToTcp;
  if (!tcp.allFinite()) return std::nullopt;
  return ChamferPlaybackPose{joints, tcp, phase};
}
