#pragma once

#include "3d/robot/kinematics/kr10kinematics.h"
#include "3d/robot/model/kr10model.h"

namespace RoboCrap3D {

class RobotPose
{
public:
  const V6d& joints() const { return m_solution.q; }
  const Kr10Kinematics::IkSolution& ikSolution() const { return m_solution; }
  const V6d& flange() const { return m_flange; }
  const std::array<M4d, LinkCount>& transforms() const { return m_transforms; }

private:
  friend class RobotPreviewState;
  RobotPose() { m_transforms.fill(M4d::Identity()); }
  // Joint coordinates, displayed flange frame and link transforms describe one pose.
  RobotPose(const Kr10Kinematics::IkSolution& solution, const V6d& flange,
            const std::array<M4d, LinkCount>& transforms)
    : m_solution(solution), m_flange(flange), m_transforms(transforms) {}
  Kr10Kinematics::IkSolution m_solution;
  V6d m_flange = V6d::Zero();
  std::array<M4d, LinkCount> m_transforms{};
};

struct RobotPoseResult
{
  std::optional<RobotPose> pose;
  QString error;
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
  RobotPoseResult initialize();
  RobotPoseResult forward(const V6d& joints) const;
  RobotPoseResult inverse(const V6d& flange) const;

private:
  friend class OccController;
  // Only the controller commits a generated candidate after presentation succeeds.
  void commit(const RobotPose& pose) { m_pose = pose; }

  Kr10Model m_model;
  Kr10Kinematics m_kinematics;
  RobotPose m_pose;
};

} // namespace RoboCrap3D
