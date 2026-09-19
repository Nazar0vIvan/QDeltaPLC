#pragma once

#include "3d/robot/kinematics/kr10kinematics.h"

namespace RoboCrap3D {

struct RobotPose
{
  V6d joints{};
  V6d flange{};
  std::array<M4d, LinkCount> transforms{};
};

class RobotPreviewState final
{
public:
  explicit RobotPreviewState(Kr10Model model);

  RobotPreviewState(const RobotPreviewState&) = delete;
  RobotPreviewState& operator=(const RobotPreviewState&) = delete;
  RobotPreviewState(RobotPreviewState&&) = delete;
  RobotPreviewState& operator=(RobotPreviewState&&) = delete;

  const Kr10Model& model() const { return m_model; }
  const RobotPose& pose() const { return m_pose; }
  bool initialize(QString& error);
  std::optional<RobotPose> forward(const V6d& joints, QString& error) const;
  std::optional<RobotPose> inverse(const V6d& flange, QString& error) const;
  void commit(const RobotPose& pose) { m_pose = pose; }

private:
  Kr10Model m_model;
  Kr10Kinematics m_kinematics;
  RobotPose m_pose;
};

} // namespace RoboCrap3D
