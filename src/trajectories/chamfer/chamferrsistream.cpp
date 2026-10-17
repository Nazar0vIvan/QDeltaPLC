#include "trajectories/chamferrsistream.h"

#include "chamfertcpsolver.h"
#include "geometry/utils.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

using namespace RoboCrap3D;

constexpr double CyclePeriod = 0.004; // s, RSI IPO_FAST controller cycle.
constexpr int SettleCycles = 50; // Zero corrections before Result = 1.
constexpr double MaximumCycles = 200000.0;
constexpr double WireScale = 1e6; // Six fixed decimal places on the wire.
constexpr double RateTolerance = 1e-6;

double unwrapDegrees(double angle, double previous)
{
  return angle + 360.0 * std::round((previous - angle) / 360.0);
}

V3d unwrapEulerDegrees(const V3d& angles, const V3d& previous)
{
  V3d result = V3d::Zero();
  for (int axis = 0; axis < 3; ++axis)
    result[axis] = unwrapDegrees(angles[axis], previous[axis]);
  return result;
}

V3d principalEulerDegrees(const M3d& rotation)
{
  // atan2 retains pitch accuracy near +/-90 degrees, where asin can lose it.
  const double cosinePitch = std::hypot(rotation(0, 0), rotation(1, 0));
  if (cosinePitch <= GeomConst::Eps)
    return {std::atan2(-rotation(0, 1), rotation(1, 1)) * GeomConst::RadToDeg,
            std::copysign(90.0, -rotation(2, 0)), 0.0};
  return {std::atan2(rotation(1, 0), rotation(0, 0)) * GeomConst::RadToDeg,
          std::atan2(-rotation(2, 0), cosinePitch) * GeomConst::RadToDeg,
          std::atan2(rotation(2, 1), rotation(2, 2)) * GeomConst::RadToDeg};
}

double rotationEntryError(const M3d& target, const V3d& angles)
{
  return (euler2rot(angles[0], angles[1], angles[2]) - target).cwiseAbs().maxCoeff();
}

std::optional<V3d> continuousEulerDegrees(const M3d& rotation, const V3d& previous)
{
  const V3d principal = principalEulerDegrees(rotation);
  V3d result = V3d::Zero();
  if (std::hypot(rotation(0, 0), rotation(1, 0)) <= GeomConst::Eps) {
    // At B=+90, A-C is fixed; at B=-90, A+C is fixed. Share its change
    // between A/C to remain closest to the preceding attitude.
    const double sign = std::copysign(1.0, -rotation(2, 0));
    const double previousCoupled = previous[0] - sign * previous[2];
    const double change = unwrapDegrees(principal[0], previousCoupled) - previousCoupled;
    result << previous[0] + 0.5 * change, unwrapDegrees(principal[1], previous[1]),
              previous[2] - sign * 0.5 * change;
  } else {
    const V3d first = unwrapEulerDegrees(principal, previous);
    const V3d second = unwrapEulerDegrees(
        V3d(principal[0] + 180.0, 180.0 - principal[1], principal[2] + 180.0), previous);
    result = (first - previous).squaredNorm() <= (second - previous).squaredNorm()
        ? first : second;
  }
  // Reject an unresolved Euler branch rather than emit a large coordinate jump.
  if (!result.allFinite() || (result - previous).cwiseAbs().maxCoeff() >= 90.0
      || rotationEntryError(rotation, result) > Kr10RotEps) return std::nullopt;
  return result;
}

// The value the controller parses from six fixed decimal places.
V6d wireValues(const V6d& values)
{
  return ((values * WireScale).array().round() / WireScale).matrix();
}

QString atTime(const QString& error, double seconds)
{
  return QStringLiteral("%1 (central t=%2 s)").arg(error).arg(seconds, 0, 'f', 6);
}

class RsiStreamSampler
{
public:
  // The saved motion outlives this worker-local sampler.
  explicit RsiStreamSampler(const ChamferMotion& motion)
    : m_motion(motion), m_solver(motion.robot()) {}

  ChamferRsiStreamResult run()
  {
    const auto& central = m_motion.centralMotion();
    if (!central) return {{}, QStringLiteral("Regenerate the path before RSI Sampling: analytical timing is missing.")};
    const auto& points = m_motion.points();
    const auto& boundaries = m_motion.boundaries();
    // Generate bounds each saved central interval h by a*h^2/8 <= 0.01 mm.
    // A controller cycle no longer than every h keeps its chord within the same bound.
    double shortestInterval = std::numeric_limits<double>::infinity();
    for (qsizetype index = boundaries[2] + 1; index <= boundaries[5]; ++index)
      shortestInterval = std::min(shortestInterval, points[index].time - points[index - 1].time);
    if (!(shortestInterval >= CyclePeriod))
      return {{}, QStringLiteral("Machining speed is too high for the 4 ms RSI chord tolerance (shortest Generate interval %1 ms).")
                      .arg(shortestInterval * 1e3, 0, 'g', 6)};
    const double duration = central->duration();
    double cycles = std::ceil(duration / CyclePeriod);
    if (cycles * CyclePeriod < duration) ++cycles;
    if (cycles > 1.0 && (cycles - 1.0) * CyclePeriod >= duration) --cycles;
    if (!std::isfinite(cycles) || cycles < 1.0 || cycles > MaximumCycles)
      return {{}, QStringLiteral("RSI sampling exceeds the 200000-cycle limit.")};
    const auto count = static_cast<qsizetype>(cycles);

    const V6d& startJoints = points[boundaries[2]].joints;
    const auto& kinematics = m_solver.kinematics();
    m_seed = kinematics.configuration(startJoints, kinematics.solveFK(startJoints));
    m_stream.duration = duration;
    m_stream.increments.reserve(count + SettleCycles);
    for (qsizetype index = 0; index <= count; ++index) {
      const double time = index == count ? duration : index * CyclePeriod;
      const QString error = index == 0 ? startCycle() : appendCycle(time);
      if (!error.isEmpty()) return {{}, atTime(error, time)};
    }
    // The cycle after the last correction carries zero motion.
    const QString stopError = accelerationError(V3d::Zero());
    if (!stopError.isEmpty()) return {{}, atTime(stopError, duration)};
    if ((m_seed.q - points[boundaries[5]].joints).cwiseAbs().maxCoeff() > 1e-6)
      return {{}, QStringLiteral("RSI endpoint does not retain the saved joint winding.")};

    m_stream.motionCycles = static_cast<int>(m_stream.increments.size());
    m_stream.increments.insert(m_stream.increments.size(), SettleCycles, V6d::Zero());
    m_stream.finalCoordinates = m_stream.initialCoordinates + m_sent;
    m_stream.stagingPoint = central->path().stagingPoint();
    return {std::move(m_stream), {}};
  }

private:

  QString startCycle()
  {
    const auto target = m_motion.centralMotion()->evaluate(0.0);
    if (!target) return QStringLiteral("Cannot evaluate the prescribed TCP target.");
    const auto inverse = m_solver.solveTcp(target->tcp, m_seed);
    if (!inverse.solution) return inverse.error;
    const M3d rotation = target->tcp.block<3, 3>(0, 0);
    const V3d angles = principalEulerDegrees(rotation);
    if (!angles.allFinite() || rotationEntryError(rotation, angles) > Kr10RotEps)
      return QStringLiteral("Cannot represent the initial TCP orientation as A/B/C.");
    m_stream.initialCoordinates << target->tcp.block<3, 1>(0, 3), angles;
    m_seed = *inverse.solution;
    m_previousPosition = target->tcp.block<3, 1>(0, 3);
    m_previousAngles = angles;
    return {};
  }

  QString appendCycle(double time)
  {
    const auto target = m_motion.centralMotion()->evaluate(time);
    if (!target) return QStringLiteral("Cannot evaluate the prescribed TCP target.");
    const auto inverse = m_solver.solveTcp(target->tcp, m_seed);
    if (!inverse.solution) return inverse.error;
    const QString speedError = jointSpeedError(inverse.solution->q);
    if (!speedError.isEmpty()) return speedError;
    const V3d position = target->tcp.block<3, 1>(0, 3);
    const V3d velocity = (position - m_previousPosition) / CyclePeriod;
    const QString accelerationFailure = accelerationError(velocity);
    if (!accelerationFailure.isEmpty()) return accelerationFailure;
    const auto angles = continuousEulerDegrees(target->tcp.block<3, 3>(0, 0), m_previousAngles);
    if (!angles) return QStringLiteral("Cannot unwrap TCP A/B/C continuously for RSI.");

    V6d coordinates = V6d::Zero();
    coordinates << position, *angles;
    // Reconcile against what was actually sent so rounding never accumulates.
    const V6d increment = wireValues(coordinates - m_stream.initialCoordinates - m_sent);
    if (!increment.allFinite()) return QStringLiteral("Nonfinite RSI correction.");
    m_sent += increment;
    m_stream.increments.append(increment);
    m_stream.minimumOffset = m_stream.minimumOffset.cwiseMin(m_sent);
    m_stream.maximumOffset = m_stream.maximumOffset.cwiseMax(m_sent);
    m_seed = *inverse.solution;
    m_previousPosition = position;
    m_previousVelocity = velocity;
    m_previousAngles = *angles;
    return {};
  }

  QString jointSpeedError(const V6d& joints) const
  {
    const V6d speed = (joints - m_seed.q).cwiseAbs() / CyclePeriod;
    const auto& robot = m_motion.robot();
    for (std::size_t axis = 0; axis < DofCount; ++axis) {
      const double limit = std::min(robot.timing.jointSpeed[axis],
                                    robot.model.joints[axis].ratedPayloadSpeedDegreesPerSecond);
      if (!std::isfinite(speed[axis]) || speed[axis] > limit * (1.0 + RateTolerance))
        return QStringLiteral("A%1 speed %2 deg/s exceeds bound %3.")
            .arg(axis + 1).arg(speed[axis], 0, 'g', 12).arg(limit);
    }
    return {};
  }

  // Change from the previous cycle's TCP velocity; the first cycle starts at rest.
  QString accelerationError(const V3d& velocity) const
  {
    const double acceleration = (velocity - m_previousVelocity).norm() / CyclePeriod;
    const double limit = m_motion.robot().timing.cartesianAcceleration;
    if (!std::isfinite(acceleration) || acceleration > limit * (1.0 + RateTolerance))
      return QStringLiteral("TCP acceleration %1 mm/s^2 exceeds bound %2.")
          .arg(acceleration, 0, 'g', 12).arg(limit);
    return {};
  }

  const ChamferMotion& m_motion;
  ChamferTcpSolver m_solver;
  ChamferRsiStream m_stream;
  Kr10Kinematics::IkSolution m_seed;
  V6d m_sent = V6d::Zero(); // Sum of increments already appended.
  V3d m_previousPosition = V3d::Zero();
  V3d m_previousVelocity = V3d::Zero();
  V3d m_previousAngles = V3d::Zero();
};

} // namespace

ChamferRsiStreamResult sampleChamferRsiStream(const ChamferMotion& motion)
{
  return RsiStreamSampler(motion).run();
}
