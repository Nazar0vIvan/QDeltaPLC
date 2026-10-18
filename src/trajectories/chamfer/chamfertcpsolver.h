#pragma once

#include "geometry/utils.h"
#include "kinematics/kr10kinematics.h"
#include "trajectories/chamfermotion.h"

#include <QString>
#include <optional>

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

struct ChamferTcpIkResult
{
  std::optional<RoboCrap3D::Kr10Kinematics::IkSolution> solution; // Configuration from the same FK.
  M4d actualTcp = M4d::Identity(); // FK flange * flangeToTcp.
  QString error;
};

// Checked TCP inverse kinematics shared by central validation and RSI sampling.
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

private:
  const ChamferRobotSetup& m_robot;
  RoboCrap3D::Kr10Kinematics m_kinematics;
  M4d m_tcpToFlange = M4d::Identity();
};
