#include "robotpreviewstate.h"

#include "3d/math/utils.h"

#include <cmath>
#include <utility>

namespace RoboCrap3D {

RobotPreviewState::RobotPreviewState(Kr10Model model)
  : m_model(std::move(model)), m_kinematics(m_model)
{}

bool RobotPreviewState::initialize(QString& error)
{
  if (!m_kinematics.isValid()) {
    error = QStringLiteral("Invalid KR10 kinematic dimensions.");
    return false;
  }

  const auto home = forward(m_model.qHome, error);
  if (!home) return false;

  const M4d expected = makeTransform(
      euler2rot(m_model.pHome[3], m_model.pHome[4], m_model.pHome[5]),
      V3d{m_model.pHome[0], m_model.pHome[1], m_model.pHome[2]});
  const M4d difference = expected.Inverted().Multiplied(home->transforms.back());

  for (int row = 1; row <= 3; ++row) {
    for (int col = 1; col <= 4; ++col) {
      const double expectedValue = row == col ? 1.0 : 0.0;
      const double tolerance = col == 4 ? GeomConst::PosEps : GeomConst::RotEps;
      if (std::abs(difference.Value(row, col) - expectedValue) > tolerance) {
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
    if (!std::isfinite(joints[i]) || joints[i] < m_model.joints[i].qMin
        || joints[i] > m_model.joints[i].qMax) {
      error = QStringLiteral("Joint %1 must be between %2 and %3 degrees.")
                  .arg(i + 1).arg(m_model.joints[i].qMin).arg(m_model.joints[i].qMax);
      return std::nullopt;
    }
  }

  RobotPose result;
  result.joints = joints;
  result.transforms = m_kinematics.solveFK(joints);
  for (const M4d& transform : result.transforms) {
    for (int row = 1; row <= 3; ++row) {
      for (int col = 1; col <= 4; ++col) {
        if (!std::isfinite(transform.Value(row, col))) {
          error = QStringLiteral("Kinematics produced a non-finite transform.");
          return std::nullopt;
        }
      }
    }
  }

  const M4d& flange = result.transforms.back();
  const V3d origin{flange.TranslationPart()};
  const EulerSolution angles = rot2euler(flange.VectorialPart());
  result.flange = {origin.X(), origin.Y(), origin.Z(), angles.A1, angles.B1, angles.C1};
  return result;
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
  const auto solution = m_kinematics.solveIK(target, m_pose.joints);
  if (!solution) {
    error = QStringLiteral("No IK solution in the current configuration. "
                           "The target may be unreachable, outside joint limits, or singular "
                           "(including the q5 = 0 home wrist position).");
    return std::nullopt;
  }
  return forward(solution->q, error);
}

} // namespace RoboCrap3D
