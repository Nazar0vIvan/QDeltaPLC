#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "3d/math/mathtypes.h"
#include "3d/robot/model/kr10model.h"

namespace RoboCrap3D {

class Kr10Kinematics final
{
public:
  struct IkSolution
  {
    V6d q{};
    uint8_t status = 0;
    uint8_t turn = 0;
  };

  explicit Kr10Kinematics(const Kr10Model& model);

  [[nodiscard]] std::array<M4d, LinkCount> solveFK(const V6d& q) const;
  [[nodiscard]] std::optional<IkSolution> solveIK(const M4d& T06,const V6d& currentQ) const;
  [[nodiscard]] bool isValid() const;

private:
  enum class ShoulderBranch { Basic, Overhead };
  enum class ElbowBranch { Positive, Negative };
  enum class WristBranch { Positive, Negative };

  struct IkBranch
  {
    ShoulderBranch shoulder = ShoulderBranch::Basic;
    ElbowBranch elbow = ElbowBranch::Positive;
    WristBranch wrist = WristBranch::Positive;
  };

  struct ArmJoints
  {
    double q1 = 0.0;
    double q2 = 0.0;
    double q3 = 0.0;
  };

  struct WristJoints
  {
    double q4 = 0.0;
    double q5 = 0.0;
    double q6 = 0.0;
  };

  [[nodiscard]] IkBranch branchFrom(const V6d& q) const;

  [[nodiscard]] std::optional<ArmJoints> solveArm(
      const V3d& wristCenter,
      const IkBranch& branch,
      const V6d& currentQ) const;

  [[nodiscard]] std::optional<WristJoints> solveWrist(
      const M4d& T06,
      const ArmJoints& arm,
      const IkBranch& branch,
      const V6d& currentQ) const;

  [[nodiscard]] std::optional<double> resolveJointAngle(
      std::size_t jointIndex,
      double deltaRad,
      double currentDeg) const;

  [[nodiscard]] bool isOverhead(const V6d& q) const;
  [[nodiscard]] bool isSolution(const M4d& targetT06, const V6d& q) const;
  [[nodiscard]] static uint8_t statusFrom(const IkBranch& branch) noexcept;
  [[nodiscard]] static uint8_t turnFrom(const V6d& q) noexcept;

private:
  const Kr10Model& m_model;
};

} // namespace RoboCrap3D
