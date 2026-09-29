#include "robotpreviewstate.h"

#include "geometry/utils.h"

#include <cmath>
#include <utility>

namespace RoboCrap3D {

RobotPreviewState::RobotPreviewState(Kr10Model model)
  : m_model(std::move(model)), m_kinematics(m_model.kinematics)
{}

bool RobotPreviewState::initialize(QString& error)
{
  if (!m_kinematics.isValid()) {
    error = QStringLiteral("Invalid KR10 kinematic dimensions.");
    return false;
  }

  const auto home = forward(m_model.kinematics.qHome, error);
  if (!home) return false;

  const M4d expected = makeTransform(
      euler2rot(m_model.kinematics.pHome[3], m_model.kinematics.pHome[4], m_model.kinematics.pHome[5]),
      V3d{m_model.kinematics.pHome[0], m_model.kinematics.pHome[1], m_model.kinematics.pHome[2]});
  const M4d difference = inverseRigidTransform(expected) * home->transforms().back();

  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 4; ++col) {
      const double expectedValue = row == col ? 1.0 : 0.0;
      const double tolerance = col == 3 ? Kr10PosEps : Kr10RotEps;
      if (std::abs(difference(row, col) - expectedValue) > tolerance) {
        error = QStringLiteral("KR10 home pose does not match its joint geometry.");
        return false;
      }
    }
  }

  commit(*home);
  return true;
}

std::optional<RobotPose> RobotPreviewState::forward(const V6d& joints, QString& error) const
{
  for (std::size_t i = 0; i < DofCount; ++i) {
    if (!std::isfinite(joints[i])) {
      error = QStringLiteral("Joint %1 must be finite.").arg(i + 1);
      return std::nullopt;
    }
    if (!IgnorePreviewJointPositionLimits
        && (joints[i] < m_model.kinematics.joints[i].qMin
            || joints[i] > m_model.kinematics.joints[i].qMax)) {
      error = QStringLiteral("Joint %1 must be between %2 and %3 degrees.")
                  .arg(i + 1).arg(m_model.kinematics.joints[i].qMin).arg(m_model.kinematics.joints[i].qMax);
      return std::nullopt;
    }
  }

  const auto transforms = m_kinematics.solveFK(joints);
  for (const M4d& transform : transforms) {
    if (!transform.allFinite()) {
      error = QStringLiteral("Kinematics produced a non-finite transform.");
      return std::nullopt;
    }
  }

  const M4d& flange = transforms.back();
  const V3d origin = flange.block<3, 1>(0, 3);
  const EulerSolution angles = rot2euler(flange.block<3, 3>(0, 0));
  const V6d frame{origin.x(), origin.y(), origin.z(), angles.A1, angles.B1, angles.C1};
  return RobotPose{joints, frame, transforms};
}

std::optional<RobotPose> RobotPreviewState::inverse(const V6d& flange, QString& error) const
{
  for (const double value : flange) {
    if (!std::isfinite(value)) {
      error = QStringLiteral("Flange pose must contain six finite numbers.");
      return std::nullopt;
    }
  }

  const M4d target = makeTransform(euler2rot(flange[3], flange[4], flange[5]),
                                 V3d{flange[0], flange[1], flange[2]});
  const auto solution = m_kinematics.solveIK(target, m_pose.joints());
  if (!solution) {
    error = QStringLiteral("No IK solution in the current configuration. "
                           "The target may be unreachable, outside joint limits, or singular "
                           "(including the q5 = 0 home wrist position).");
    return std::nullopt;
  }
  return forward(solution->q, error);
}

} // namespace RoboCrap3D
