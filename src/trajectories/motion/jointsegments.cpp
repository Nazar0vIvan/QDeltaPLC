#include "jointsegments.h"

#include "geometry/utils.h"
#include "kinematics/kr10kinematics.h"

#include <cmath>
#include <utility>

using namespace RoboCrap3D;

namespace {

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
    const double speedBound = robot.timing.jointSpeed[axis];
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

} // namespace

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

namespace {

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

// Peak speed, ramps and cruise of a profile whose poses, limits and path length are set.
std::optional<LinearProfile> timedProfile(LinearProfile result)
{
  const LinearLimits& limits = result.limits;
  if (!std::isfinite(result.length) || result.length <= GeomConst::Eps) return std::nullopt;
  if (limits.acceleration == 0.0) {
    result.peak = limits.feed;
    result.ramp = 0.0;
    result.cruise = result.length / limits.feed;
    const bool valid = std::isfinite(result.peak) && result.peak > 0.0 && std::isfinite(result.cruise) && result.cruise > 0.0;
    return valid ? std::optional<LinearProfile>{result} : std::nullopt;
  }
  // Compare requested ramp distance before multiplying acceleration by length.
  const double requestedRamp = limits.feed / limits.acceleration;
  const bool triangular = !std::isfinite(requestedRamp)
      || limits.feed * requestedRamp >= result.length;
  result.peak = triangular ? std::sqrt(result.length) * std::sqrt(limits.acceleration)
                           : limits.feed;
  result.ramp = result.peak / limits.acceleration;
  result.cruise = triangular ? 0.0 : (result.length - result.peak * result.ramp) / result.peak;
  if (!std::isfinite(result.peak) || result.peak <= 0.0 || !std::isfinite(result.ramp)
      || result.ramp <= 1e-9 || !std::isfinite(result.cruise) || result.cruise < 0.0
      || !std::isfinite(result.duration()) || result.duration() <= result.ramp
      || (result.cruise > 0.0 && result.ramp + result.cruise <= result.ramp))
    return std::nullopt;
  return result;
}

} // namespace

RobotSetupResult ratedRobotSetup(const ChamferRobotSetup& robot)
{
  if (!validRobot(robot)) return {{}, QStringLiteral("Invalid robot model, rated joint speeds, HOME limits or flange-to-TCP transform.")};
  ChamferRobotSetup rated = robot;
  // The model is authoritative, including regeneration of paths saved with the old cap.
  for (std::size_t joint = 0; joint < DofCount; ++joint)
    rated.timing.jointSpeed[joint] = robot.model.joints[joint].ratedPayloadSpeedDegreesPerSecond;
  if (!validTiming(rated.timing)) return {{}, QStringLiteral("Feeds, transfer acceleration and simulation limits must be finite and positive; acceleration distance and auxiliary scale must be in (0, 1].")};
  // KRL assigns both transfer values to $VEL.CP/$ACC.CP, which the controller rejects above these ceilings.
  if (rated.timing.transferFeed > MaximumTransferFeed
      || rated.timing.transferAcceleration > rated.timing.cartesianAcceleration)
    return {{}, QStringLiteral("LIN transfer exceeds $VEL_MA.CP = %1 mm/s or $ACC_MA.CP = %2 mm/s^2.")
                    .arg(MaximumTransferFeed).arg(rated.timing.cartesianAcceleration)};
  return {std::move(rated), {}};
}

double smoothProgress(double u)
{
  return u * u * u * (10.0 + u * (-15.0 + 6.0 * u));
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

double ptpDuration(const V6d& delta, const ChamferTimingParameters& timing)
{
  // Quintic synchronized progress: max s'=1.875, max |s''|=10/sqrt(3).
  double duration = 0.001; // A coincident HOME endpoint retains a finite phase.
  for (int joint = 0; joint < 6; ++joint) {
    duration = std::max(duration, 1.875 * std::abs(delta[joint])
        / (timing.jointSpeed[joint] * timing.auxiliaryScale));
    duration = std::max(duration, std::sqrt((10.0 / std::sqrt(3.0)) * std::abs(delta[joint])
        / (timing.jointAcceleration[joint] * timing.auxiliaryScale)));
  }
  return duration;
}

std::optional<LinearProfile> linearProfile(const M4d& first, const M4d& last, const LinearLimits& limits)
{
  LinearProfile result;
  result.first = first;
  result.last = last;
  result.limits = limits;
  if (!rigid(result.first) || !rigid(result.last)) return std::nullopt;
  result.firstRotation = Eigen::Quaterniond(M3d(result.first.block<3, 3>(0, 0))).normalized();
  result.lastRotation = Eigen::Quaterniond(M3d(result.last.block<3, 3>(0, 0))).normalized();
  result.length = (result.last.block<3, 1>(0, 3) - result.first.block<3, 1>(0, 3)).stableNorm();
  return timedProfile(std::move(result));
}

std::optional<LinearProfile> circularProfile(const LinearProfile& chord, const V3d& centre)
{
  const V3d from = chord.first.block<3, 1>(0, 3) - centre;
  const V3d to = chord.last.block<3, 1>(0, 3) - centre;
  const V3d normal = from.cross(to);
  const double radius = from.stableNorm();
  if (!centre.allFinite() || !std::isfinite(radius) || radius <= GeomConst::Eps
      || std::abs(to.stableNorm() - radius) > TransferPositionTolerance
      || !(normal.stableNorm() > 1e-6 * radius * radius))
    return std::nullopt;
  LinearProfile result = chord;
  result.centre = centre;
  result.axis = normal.normalized();
  result.angle = std::atan2(normal.stableNorm(), from.dot(to));
  result.length = radius * result.angle;
  return timedProfile(std::move(result));
}

LinearSchedule linearSchedule(const LinearProfile& profile)
{
  LinearSchedule result;
  const std::array<double, 4> stops{0.0, profile.ramp, profile.ramp + profile.cruise, profile.duration()};
  std::array<int, 3> intervals{};
  qsizetype plannedPoints = 1;
  // Count all points before IK. A straight LIN has zero normal acceleration; an arc adds the
  // bound peak^2 / radius to every stage.
  const double normal = profile.angle == 0.0 ? 0.0 : profile.peak * profile.peak * profile.angle / profile.length;
  for (std::size_t stage = 0; stage < intervals.size(); ++stage) {
    const double duration = stops[stage + 1] - stops[stage];
    if (duration == 0.0) continue;
    const double acceleration = (stage == 1 ? 0.0 : profile.limits.acceleration) + normal;
    const double count = std::max(1.0, std::ceil(duration * std::sqrt(acceleration / (8.0 * TransferPositionTolerance))));
    if (!std::isfinite(duration) || duration <= 0.0 || !std::isfinite(count)
        || count > 200000 - plannedPoints || !(duration / count > 1e-9)) {
      result.error = QStringLiteral("Transfer schedule exceeds numerical range or 200000 points.");
      return result;
    }
    intervals[stage] = static_cast<int>(count);
    plannedPoints += intervals[stage];
  }
  result.localTimes.reserve(plannedPoints - 1);
  for (std::size_t stage = 0; stage < intervals.size(); ++stage)
    for (int index = 1; index <= intervals[stage]; ++index)
      result.localTimes.append(index == intervals[stage] ? stops[stage + 1]
          : stops[stage] + (stops[stage + 1] - stops[stage])
              * (static_cast<double>(index) / intervals[stage]));
  return result;
}

JointSplineResult clampedJointSpline(const QVector<ChamferJointPoint>& points, const ChamferRobotSetup& robot)
{
  const qsizetype count = points.size();
  if (count < 2) return {{}, QStringLiteral("Missing joint spline points.")};
  QVector<double> steps(count - 1);
  QVector<V6d> slopes(count - 1, V6d::Zero());
  for (qsizetype i = 0; i + 1 < count; ++i) {
    steps[i] = points[i + 1].time - points[i].time;
    if (!std::isfinite(steps[i]) || steps[i] <= 1e-9)
      return {{}, QStringLiteral("Joint spline timestamps cannot be resolved.")};
    slopes[i] = (points[i + 1].joints - points[i].joints) / steps[i];
    if (!slopes[i].allFinite()) return {{}, QStringLiteral("Nonfinite joint spline secant speed.")};
  }
  // Row-scaled clamped spline system for second derivatives (deg/s^2).
  // Endpoint slopes are zero. The same unknown is shared across phase boundaries.
  QVector<double> upper(count, 0.0);
  QVector<V6d> second(count, V6d::Zero());
  upper[0] = 0.5;
  second[0] = 3.0 * slopes[0] / steps[0];
  if (!second[0].allFinite()) return {{}, QStringLiteral("Nonfinite joint spline boundary equation.")};
  for (qsizetype i = 1; i < count; ++i) {
    double lower = 1.0;
    double next = 0.0;
    V6d rhs = V6d::Zero();
    if (i + 1 == count) rhs = -6.0 * slopes[i - 1] / steps[i - 1];
    else {
      const double sum = steps[i - 1] + steps[i];
      if (!std::isfinite(sum)) return {{}, QStringLiteral("Nonfinite joint spline interval sum.")};
      lower = steps[i - 1] / sum;
      next = steps[i] / sum;
      rhs = 6.0 * (slopes[i] - slopes[i - 1]) / sum;
    }
    const double diagonal = 2.0 - lower * upper[i - 1];
    if (!std::isfinite(diagonal) || diagonal <= 0.0 || !rhs.allFinite())
      return {{}, QStringLiteral("Invalid joint spline equation.")};
    upper[i] = next / diagonal;
    second[i] = (rhs - lower * second[i - 1]) / diagonal;
    if (!second[i].allFinite()) return {{}, QStringLiteral("Nonfinite joint spline solution.")};
  }
  for (qsizetype i = count - 1; i > 0; --i) {
    second[i - 1] -= upper[i - 1] * second[i];
    if (!second[i - 1].allFinite()) return {{}, QStringLiteral("Nonfinite joint spline back substitution.")};
  }
  JointSplineResult result;
  result.coefficients.reserve(count - 1);
  for (qsizetype i = 0; i + 1 < count; ++i) {
    const double square = steps[i] * steps[i];
    const V6d delta = points[i + 1].joints - points[i].joints;
    const std::array<V6d, 4> coefficients{
        points[i].joints,
        V6d(delta - (square / 6.0) * (2.0 * second[i] + second[i + 1])),
        V6d((square / 2.0) * second[i]),
        V6d((square / 6.0) * (second[i + 1] - second[i]))};
    const QString error = jointSplineIntervalError(coefficients, steps[i], robot);
    if (!error.isEmpty())
      return {{}, QStringLiteral("%1 (motion interval [%2, %3] s).")
          .arg(error).arg(points[i].time, 0, 'g', 12).arg(points[i + 1].time, 0, 'g', 12)};
    result.coefficients.append(coefficients);
  }
  return result;
}
