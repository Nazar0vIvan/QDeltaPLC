#pragma once

#include "kinematics/kr10kinematics.h"
#include "robot/robotdefinition.h"
#include "scene/sceneendeffectors.h"

#include <QObject>
#include <memory>

class ScenePlayback;

namespace RoboCrap3D {

class OccController;
struct DrawingPose;

// BASE-relative link transforms of one FK pose, or the reason it was rejected.
struct RobotPoseResult
{
  std::optional<QList<M4d>> links;
  QString error;
};

class RobotController final : public QObject
{
  Q_OBJECT
public:
  RobotController(OccController* viewport, SceneEndEffectors* effectors, ScenePlayback* playback);
  ~RobotController() override;
  Q_DISABLE_COPY_MOVE(RobotController)

  ViewportOperationResult loadRobot();
  ViewportOperationResult applyPlayback(const V6d& joints, const M4d& flangeToTcp);

signals:
  void message(const QString& text, bool error);

private:
  bool isReady() const;
  RobotPoseResult initialize();
  RobotPoseResult forward(const V6d& joints) const;
  DrawingPose drawingPose(const QList<M4d>& links, const std::optional<M4d>& checkedTcp = std::nullopt) const;
  ViewportOperationResult applyPose(const QList<M4d>& links, const std::optional<M4d>& checkedTcp);
  ViewportOperationResult failure(const QString& error);
  void setError(const QString& error);
  void synchronizeReadiness();
  void synchronizeTools();

  OccController* m_viewport;
  SceneEndEffectors* m_endEffectors;
  ScenePlayback* m_playback;
  const RobotDefinition m_model = RobotDefinition::kr10();
  std::unique_ptr<Kr10Kinematics> m_kinematics;
  std::optional<QList<M4d>> m_pose;
  std::optional<QList<M4d>> m_initialPose;
  QString m_error;
};

} // namespace RoboCrap3D
