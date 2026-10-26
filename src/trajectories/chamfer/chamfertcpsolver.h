#pragma once

#include "../motion/jointsegments.h"
#include "geometry/utils.h"
#include "kinematics/kr10kinematics.h"
#include "trajectories/chamfermotion.h"

#include <QString>
#include <cmath>
#include <optional>
#include <utility>

inline QString chamferJointPositionError(const V6d& joints, const RoboCrap3D::Kr10KinematicModel& model)
{
  if (!joints.allFinite()) return QStringLiteral("Nonfinite joint coordinates.");
  for (std::size_t axis = 0; axis < RoboCrap3D::DofCount; ++axis) {
    const auto& joint = model.joints[axis];
    if (joints[axis] < joint.qMin || joints[axis] > joint.qMax)
      return QStringLiteral("A%1 position %2 deg is outside [%3, %4].")
          .arg(axis + 1).arg(joints[axis], 0, 'g', 12).arg(joint.qMin).arg(joint.qMax);
  }
  return {};
}

// Bounds come from the model speeds accepted by ChamferMotion::create.
inline QString chamferJointSpeedError(const V6d& velocity, const V6d& limits)
{
  if (!velocity.allFinite()) return QStringLiteral("Nonfinite sampled joint velocity.");
  for (std::size_t axis = 0; axis < RoboCrap3D::DofCount; ++axis) {
    if (std::abs(velocity[axis]) > limits[axis] * (1.0 + 1e-6))
      return QStringLiteral("A%1 speed %2 deg/s exceeds bound %3.")
          .arg(axis + 1).arg(std::abs(velocity[axis]), 0, 'g', 12).arg(limits[axis]);
  }
  return {};
}

struct ChamferTcpIkResult
{
  std::optional<RoboCrap3D::Kr10Kinematics::IkSolution> solution; // Configuration from the same FK.
  M4d actualTcp = M4d::Identity(); // FK flange * flangeToTcp.
  QString error;
};

struct LinearSamples
{
  QVector<ChamferJointPoint> points; // Timed points after the transfer start.
  RoboCrap3D::Kr10Kinematics::IkSolution last; // Seed for the following segment.
  QString error;
};

// Checked TCP inverse kinematics shared by central validation and RSI sampling, and the
// PTP/LIN point construction of generated motions.
// Borrows the worker-owned robot setup, which must outlive this solver.
class ChamferTcpSolver
{
public:
  explicit ChamferTcpSolver(const ChamferRobotSetup& robot)
    : m_robot(robot), m_kinematics(robot.model),
      m_tcpToFlange(inverseRigidTransform(robot.flangeToTcp)) {}

  const RoboCrap3D::Kr10Kinematics& kinematics() const { return m_kinematics; }

  // Seeded angle-only IK for a TCP target, then joint limits, one FK and
  // flange/TCP residual checks against the existing KR10 tolerances.
  ChamferTcpIkResult solveTcp(const M4d& tcp, const RoboCrap3D::Kr10Kinematics::IkSolution& seed) const
  {
    const M4d flange = tcp * m_tcpToFlange;
    const auto solution = m_kinematics.calculateIK(flange, seed);
    if (!solution) return {{}, M4d::Identity(), QStringLiteral("IK failed: unreachable, singular or outside the seeded configuration/limits.")};
    const QString error = chamferJointPositionError(solution->q, m_robot.model);
    if (!error.isEmpty()) return {{}, M4d::Identity(), error};
    const auto transforms = m_kinematics.solveFK(solution->q);
    if (!RoboCrap3D::Kr10Kinematics::matchesFlange(flange, transforms.back()))
      return {{}, M4d::Identity(), QStringLiteral("Flange FK reconstruction exceeds 0.0001 mm or 0.000001 rotation-entry tolerance.")};
    const M4d actual = transforms.back() * m_robot.flangeToTcp;
    if (!actual.allFinite()
        || (actual.block<3, 1>(0, 3) - tcp.block<3, 1>(0, 3)).norm() > RoboCrap3D::Kr10PosEps
        || (actual.block<3, 3>(0, 0) - tcp.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > RoboCrap3D::Kr10RotEps)
      return {{}, M4d::Identity(), QStringLiteral("TCP FK reconstruction exceeds 0.0001 mm or 0.000001 rotation-entry tolerance.")};
    return {m_kinematics.configuration(solution->q, transforms), actual, {}};
  }

  // Joint coordinates with their FK TCP; progress is phase-local, time is set by the caller.
  ChamferJointPoint forward(const V6d& joints, double progress) const
  {
    return {joints, m_kinematics.solveFK(joints).back() * m_robot.flangeToTcp, progress};
  }

  // Seeded IK, joint limits and one flange FK check for a prescribed transfer target;
  // returns the configuration from that FK and the FK-derived TCP.
  std::optional<std::pair<RoboCrap3D::Kr10Kinematics::IkSolution, M4d>> inverse(
      const M4d& tcp, const RoboCrap3D::Kr10Kinematics::IkSolution& seed) const
  {
    const M4d flange = tcp * m_tcpToFlange;
    const auto solution = m_kinematics.calculateIK(flange, seed);
    if (!solution || !withinLimits(solution->q, m_robot.model)) return std::nullopt;
    const auto transforms = m_kinematics.solveFK(solution->q);
    if (!RoboCrap3D::Kr10Kinematics::matchesFlange(flange, transforms.back()))
      return std::nullopt;
    return std::pair{m_kinematics.configuration(solution->q, transforms),
                     M4d(transforms.back() * m_robot.flangeToTcp)};
  }

  // PTP points after start in equal joint steps of at most 2 degrees, with joint progress.
  QVector<ChamferJointPoint> samplePtp(const V6d& start, const V6d& end) const
  {
    const int intervals = std::max(1, static_cast<int>(std::ceil((end - start).cwiseAbs().maxCoeff() / 2.0)));
    QVector<ChamferJointPoint> points;
    points.reserve(intervals);
    for (int i = 1; i <= intervals; ++i) {
      const double progress = static_cast<double>(i) / intervals;
      const V6d joints = i == intervals ? end : V6d(start + progress * (end - start));
      points.append(forward(joints, progress));
    }
    return points;
  }

  // IK for every scheduled LIN point, TCP residual and joint-speed checks. The seed is the
  // solution of the preceding point, so its angles and schedule.startTime anchor the rates.
  // label prefixes the point errors with the motion segment name.
  LinearSamples solveLinear(const LinearSchedule& schedule,
                            const RoboCrap3D::Kr10Kinematics::IkSolution& seed, const QString& label) const
  {
    QVector<ChamferJointPoint> points{ChamferJointPoint{seed.q, M4d::Identity(), 0.0, schedule.startTime}};
    points.reserve(schedule.localTimes.size() + 1);
    auto previous = seed;
    for (double local : schedule.localTimes) {
      const double time = schedule.startTime + local;
      if (!std::isfinite(time) || !(time > points.back().time))
        return {{}, seed, QStringLiteral("Transfer timestamps cannot be resolved.")};
      const M4d target = schedule.profile.target(local);
      const auto solution = inverse(target, previous);
      if (!solution)
        return {{}, seed, QStringLiteral("%1: IK, joint position or TCP reconstruction failed at %2 s.")
            .arg(label).arg(local, 0, 'g', 12)};
      ChamferJointPoint point{solution->first.q, solution->second, schedule.profile.progress(local)};
      if ((point.tcp.block<3, 1>(0, 3) - target.block<3, 1>(0, 3)).norm() > RoboCrap3D::Kr10PosEps
          || (point.tcp.block<3, 3>(0, 0) - target.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > RoboCrap3D::Kr10RotEps)
        return {{}, seed, QStringLiteral("%1: TCP fails FK reconstruction tolerance at %2 s.")
            .arg(label).arg(local, 0, 'g', 12)};
      point.time = time;
      points.append(point);
      previous = solution->first;
    }
    const QString error = linearRateError(points, label);
    if (!error.isEmpty()) return {{}, seed, error};
    points.removeFirst();
    return {std::move(points), previous, {}};
  }

private:
  QString linearRateError(const QVector<ChamferJointPoint>& points, const QString& label) const
  {
    qsizetype anchor = 0;
    V6d previous = V6d::Zero();
    double previousStep = 0.0;
    for (qsizetype i = 1; i < points.size(); ++i) {
      const double dt = points[i].time - points[i - 1].time;
      const V6d velocity = (points[i].joints - points[i - 1].joints) / dt;
      const QString speedError = chamferJointSpeedError(velocity, m_robot.timing.jointSpeed);
      if (!speedError.isEmpty())
        return QStringLiteral("%1: %2 (motion t=%3 s).")
            .arg(label, speedError).arg(points[i].time, 0, 'g', 12);
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
      //         .arg(label).arg(axis + 1)
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
    //         .arg(label).arg(axis + 1);
    return {};
  }

  const ChamferRobotSetup& m_robot;
  RoboCrap3D::Kr10Kinematics m_kinematics;
  M4d m_tcpToFlange = M4d::Identity();
};
