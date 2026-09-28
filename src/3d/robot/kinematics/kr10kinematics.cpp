#include "kr10kinematics.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <QDebug>

#include "geometry/utils.h"

namespace RoboCrap3D {

namespace {

double posErr(const M4d& T)
{
  return T.block<3, 1>(0, 3).norm();
}

double rotErr(const M4d& T)
{
  double maxErr = 0.0;

  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      const double want = row == col ? 1.0 : 0.0;
      maxErr = std::max(maxErr, std::abs(T(row, col) - want));
    }
  }

  return maxErr;
}
} // namespace

Kr10Kinematics::Kr10Kinematics(const Kr10KinematicModel& model) : m_model(model)
{}

bool Kr10Kinematics::isValid() const
{
  const Kr10KinematicModel::IkParams& ik = m_model.ik;
  const double b = std::hypot(ik.by, ik.bx);

  return ik.a > GeomConst::Eps && b > GeomConst::Eps && std::abs(ik.dF) > GeomConst::Eps;
}

std::array<M4d, LinkCount> Kr10Kinematics::solveFK(const V6d& q) const
{
  std::array<M4d, LinkCount> T0i{};

  M4d T = M4d::Identity();
  T0i[0] = T;

  for (std::size_t i = 0; i < DofCount; ++i) {
    const JointModel& joint = m_model.joints[i];
    const double dq = q[i] - m_model.qHome[i];

    T = (T * joint.localTransform * makeRotation(dq, joint.axis)).eval();
    T0i[i + 1] = T;
  }

  return T0i;
}

std::optional<Kr10Kinematics::IkSolution>
Kr10Kinematics::solveIK(const M4d& targetT06, const V6d& currentQ) const
{
  if (!isValid()) {
    qWarning() << "Cannot solve KR10 IK: invalid kinematic parameters";
    return std::nullopt;
  }

  const IkBranch branch = branchFrom(currentQ);

  const V3d flangeOrigin = targetT06.block<3, 1>(0, 3);
  const V3d flangeZ = targetT06.block<3, 1>(0, 2);

  const V3d wristCenter = flangeOrigin - flangeZ * m_model.ik.dF;

  const std::optional<ArmJoints> arm = solveArm(wristCenter, branch, currentQ);
  if (!arm) return std::nullopt;

  const std::optional<WristJoints> wrist = solveWrist(targetT06, *arm, branch, currentQ);
  if (!wrist) return std::nullopt;

  const V6d q {
      arm->q1,
      arm->q2,
      arm->q3,
      wrist->q4,
      wrist->q5,
      wrist->q6
  };

  if (!isSolution(targetT06, q))
    return std::nullopt;

  return IkSolution{
    q,
    statusFrom(branch),
    turnFrom(q)
  };
}

Kr10Kinematics::IkBranch
Kr10Kinematics::branchFrom(const V6d &q) const
{
  const double alpha = std::atan2(m_model.ik.bx, m_model.ik.by);

  const JointModel& joint3 = m_model.joints[2];
  const double dq3 = (q[2] - m_model.qHome[2]) * GeomConst::DegToRad / joint3.axis.z();

  const double elbowRad = alpha - GeomConst::Pi / 2.0 - dq3;

  const JointModel& joint5 = m_model.joints[4];
  const double dq5 = (q[4] - m_model.qHome[4]) * GeomConst::DegToRad / joint5.axis.z();

  return {
      isOverhead(q) ? ShoulderBranch::Overhead : ShoulderBranch::Basic,
      elbowRad <= 0.0 ? ElbowBranch::Negative : ElbowBranch::Positive,
      dq5 < 0.0 ? WristBranch::Negative : WristBranch::Positive
  };
}


std::optional<Kr10Kinematics::ArmJoints>
Kr10Kinematics::solveArm(const V3d &wristCenter,
                         const IkBranch &branch,
                         const V6d &currentQ) const
{
  const Kr10KinematicModel::IkParams& ik = m_model.ik;

  // q1
  const double radialDistance = std::hypot(wristCenter.x(), wristCenter.y());
  if (radialDistance <= GeomConst::Eps)
    return std::nullopt;

  const bool overhead = branch.shoulder == ShoulderBranch::Overhead;

  const double dq1 = std::atan2(wristCenter.y(), wristCenter.x()) + (overhead ? GeomConst::Pi : 0.0);

  const double signedRadius = overhead ? -radialDistance : radialDistance;

  const std::optional<double> q1 = resolveJointAngle(0, dq1, currentQ[0]);
  if (!q1) return std::nullopt;

  // q2
  const double planarX = signedRadius - ik.sx;
  const double planarZ = wristCenter.z() - ik.sz;
  const double distanceSquared = planarX * planarX + planarZ * planarZ;

  if (!std::isfinite(distanceSquared) || distanceSquared <= GeomConst::Eps * GeomConst::Eps)
    return std::nullopt;

  const double forearmLength = std::hypot(ik.bx, ik.by);
  const double cosElbow = (distanceSquared - ik.a * ik.a - forearmLength * forearmLength) / (2.0 * ik.a * forearmLength);

  if (!std::isfinite(cosElbow) || cosElbow < -1.0 - GeomConst::Eps || cosElbow >  1.0 + GeomConst::Eps) {
    return std::nullopt;
  }

  const double elbowMagnitudeRad = std::acos(std::clamp(cosElbow, -1.0, 1.0));
  const double elbowRad = branch.elbow == ElbowBranch::Positive ? elbowMagnitudeRad : -elbowMagnitudeRad;
  const double upperArmRad = std::atan2(planarZ, planarX)- std::atan2(forearmLength * std::sin(elbowRad), ik.a + forearmLength * std::cos(elbowRad));
  const double dq2 = GeomConst::Pi / 2.0 - upperArmRad;
  const std::optional<double> q2 = resolveJointAngle(1, dq2, currentQ[1]);

  // q3
  const double alpha = std::atan2(ik.bx, ik.by);
  const double dq3 = alpha - GeomConst::Pi / 2.0 - elbowRad;
  const std::optional<double> q3 = resolveJointAngle(2, dq3, currentQ[2]);

  if (!q2 || !q3) return std::nullopt;

  return ArmJoints{*q1, *q2, *q3};
}

std::optional<Kr10Kinematics::WristJoints>
Kr10Kinematics::solveWrist(const M4d &targetT06,
                           const ArmJoints &arm,
                           const IkBranch &branch,
                           const V6d &currentQ) const
{
  V6d q123 = m_model.qHome;
  q123[0] = arm.q1;
  q123[1] = arm.q2;
  q123[2] = arm.q3;

  const std::array<M4d, LinkCount> transforms = solveFK(q123);
  const M4d T36 = inverseRigidTransform(transforms[3]) * targetT06;

  const double rawCosQ5 = -T36(1, 2);

  if (!std::isfinite(rawCosQ5) || rawCosQ5 < -1.0 - GeomConst::Eps || rawCosQ5 >  1.0 + GeomConst::Eps) {
    return std::nullopt;
  }

  const double q5MagnitudeRad = std::acos(std::clamp(rawCosQ5, -1.0, 1.0));

  const double dq5 = branch.wrist == WristBranch::Positive ? q5MagnitudeRad : -q5MagnitudeRad;

  const double sinQ5 = std::sin(dq5);

  // A5 = 0 is a wrist singularity:
  // A4 and A6 cannot be determined independently.
  if (std::abs(sinQ5) <= GeomConst::Eps)
    return std::nullopt;

  const double dq4 = std::atan2(-T36(2, 2) / sinQ5, T36(0, 2) / sinQ5);
  const double dq6 = std::atan2(-T36(1, 1) / sinQ5, T36(1, 0) / sinQ5);

  const std::optional<double> q4 = resolveJointAngle(3, dq4, currentQ[3]);
  const std::optional<double> q5 = resolveJointAngle(4, dq5, currentQ[4]);
  const std::optional<double> q6 = resolveJointAngle(5, dq6, currentQ[5]);

  if (!q4 || !q5 || !q6)
    return std::nullopt;

  return WristJoints{*q4, *q5, *q6};
}

std::optional<double> Kr10Kinematics::resolveJointAngle(std::size_t jointIndex, double deltaRad, double currentDeg) const
{
  const JointModel& joint = m_model.joints[jointIndex];

  const double angleDeg = m_model.qHome[jointIndex]+ deltaRad * joint.axis.z() * GeomConst::RadToDeg;
  if (!std::isfinite(angleDeg) || !std::isfinite(currentDeg)) return std::nullopt;

  const int firstTurn = static_cast<int>(std::ceil((joint.qMin - angleDeg) / 360.0));

  const int lastTurn = static_cast<int>(std::floor((joint.qMax - angleDeg) / 360.0));

  if (firstTurn > lastTurn) return std::nullopt;

  const int nearestTurn = static_cast<int>(std::round((currentDeg - angleDeg) / 360.0));

  const int turn = std::clamp(nearestTurn, firstTurn, lastTurn);

  return angleDeg + 360.0 * static_cast<double>(turn);
}

bool Kr10Kinematics::isOverhead(const V6d& q) const
{
  const std::array<M4d, LinkCount> transforms = solveFK(q);

  const M4d& T01 = transforms[1];
  const M4d& T06 = transforms.back();

  const V3d flangeOrigin = T06.block<3, 1>(0, 3);
  const V3d flangeZ = T06.block<3, 1>(0, 2);

  const V3d wristCenter = flangeOrigin - flangeZ * m_model.ik.dF;

  const V3d a1Origin = T01.block<3, 1>(0, 3);
  const V3d a1X = T01.block<3, 1>(0, 0);

  const double wristXInA1 = (wristCenter - a1Origin).dot(a1X);

  return wristXInA1 < 0.0;
}

bool Kr10Kinematics::isSolution(const M4d& targetT06, const V6d& q) const
{
  const M4d actualT06 = solveFK(q).back();
  const M4d errorTransform = inverseRigidTransform(targetT06) * actualT06;

  return
      posErr(errorTransform) <= Kr10PosEps &&
      rotErr(errorTransform) <= Kr10RotEps;
}

uint8_t Kr10Kinematics::statusFrom(const IkBranch &branch) noexcept
{
  uint8_t status = 0;

  if (branch.shoulder == ShoulderBranch::Overhead)
    status |= static_cast<uint8_t>(1u << 0);

  if (branch.elbow == ElbowBranch::Negative)
    status |= static_cast<uint8_t>(1u << 1);

  if (branch.wrist == WristBranch::Negative)
    status |= static_cast<uint8_t>(1u << 2);

  return status;
}

uint8_t Kr10Kinematics::turnFrom(const V6d& q) noexcept
{
  uint8_t turn = 0;

  for (std::size_t i = 0; i < q.size(); ++i) {
    if (q[i] < 0.0)
      turn |= static_cast<uint8_t>(1u << i);
  }

  return turn;
}

} // namespace RoboCrap3D
