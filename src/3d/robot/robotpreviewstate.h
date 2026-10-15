#pragma once

#include "3d/robot/kinematics/kr10kinematics.h"
#include "3d/robot/model/kr10model.h"
#include "scene/sceneendeffectors.h"

#include <QObject>
#include <QVariantList>
#include <QJSEngine>
#include <QPointer>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <memory>

class SceneMachining;

namespace RoboCrap3D {

class OccController;
struct DrawingPose;

class RobotPose
{
public:
  const V6d& joints() const { return m_solution.q; }
  const Kr10Kinematics::IkSolution& ikSolution() const { return m_solution; }
  const V6d& flange() const { return m_flange; }
  const QList<M4d>& transforms() const { return m_transforms; }

private:
  friend class RobotController;
  // Joint coordinates, displayed flange frame and link transforms describe one pose.
  RobotPose(const Kr10Kinematics::IkSolution& solution, const V6d& flange,
            const QList<M4d>& transforms)
    : m_solution(solution), m_flange(flange), m_transforms(transforms) {}
  Kr10Kinematics::IkSolution m_solution;
  V6d m_flange = V6d::Zero();
  QList<M4d> m_transforms;
};

struct RobotPoseResult
{
  std::optional<RobotPose> pose;
  QString error;
};

struct RobotOperationResult
{
  Q_GADGET
  QML_VALUE_TYPE(robotOperationResult)
  Q_PROPERTY(bool accepted MEMBER accepted)
  Q_PROPERTY(QVariantList values MEMBER values)
  Q_PROPERTY(QString error MEMBER error)
public:
  bool accepted = false;
  QVariantList values;
  QString error;
};

class RobotController final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
  Q_PROPERTY(QVariantList jointAngles READ jointAngles NOTIFY poseChanged)
  Q_PROPERTY(QVariantList flangePose READ flangePose NOTIFY poseChanged)
  Q_PROPERTY(QList<double> tcpPose READ tcpPose NOTIFY tcpPoseChanged)
public:
  // Drawing acceptance, accepted tool settings and machining are distinct collaborators.
  RobotController(OccController* viewport, SceneEndEffectors* effectors, SceneMachining* machining);
  ~RobotController() override;
  Q_DISABLE_COPY_MOVE(RobotController)

  bool isReady() const;
  bool isLoading() const;
  QString errorString() const { return m_error; }
  QVariantList jointAngles() const;
  QVariantList flangePose() const;
  QList<double> tcpPose() const;
  Q_INVOKABLE RobotOperationResult loadRobot();
  Q_INVOKABLE RobotOperationResult solveFK(const QVariantList& joints);
  Q_INVOKABLE RobotOperationResult solveIK(const QVariantList& flange);
  Q_INVOKABLE RobotOperationResult solveTcpIK(const QVariantList& tcp);
  Q_INVOKABLE void reloadEndEffector(SceneEndEffectors::Tool tool);
  RobotOperationResult applyPlayback(const V6d& joints, const M4d& flangeToTcp);
  bool setActiveTool(SceneEndEffectors::Tool tool);

signals:
  void readyChanged();
  void loadingChanged();
  void errorStringChanged();
  void poseChanged();
  void tcpPoseChanged();
  void message(const QString& text, bool error);

private:
  RobotPoseResult initialize();
  RobotPoseResult forward(const V6d& joints) const;
  RobotPoseResult inverse(const V6d& flange) const;
  DrawingPose drawingPose(const RobotPose& pose, const std::optional<M4d>& checkedTcp = std::nullopt) const;
  RobotOperationResult applyPose(const RobotPose& pose, const std::optional<M4d>& checkedTcp = std::nullopt);
  RobotOperationResult solve(const QVariantList& values, bool inverse);
  RobotOperationResult failure(const QString& error);
  void setError(const QString& error);
  void onViewportError();
  void synchronizeReadiness();
  void synchronizeTools();

  OccController* m_viewport;
  SceneEndEffectors* m_endEffectors;
  SceneMachining* m_machining;
  RobotDefinition m_model;
  std::unique_ptr<Kr10Kinematics> m_kinematics;
  std::optional<RobotPose> m_pose;
  std::optional<RobotPose> m_initialPose;
  QString m_error;
};

} // namespace RoboCrap3D

struct RobotControllerQml
{
  Q_GADGET
  QML_FOREIGN(RoboCrap3D::RobotController)
  QML_NAMED_ELEMENT(RobotController)
  QML_SINGLETON
public:
  inline static RoboCrap3D::RobotController* s_inst = nullptr;
  static RoboCrap3D::RobotController* create(QQmlEngine*, QJSEngine* engine)
  {
    if (!s_inst || engine->thread() != s_inst->thread() || (s_engine && s_engine != engine))
      qFatal("RobotController must be used by one GUI-thread QML engine at a time");
    s_engine = engine;
    QJSEngine::setObjectOwnership(s_inst, QJSEngine::CppOwnership);
    return s_inst;
  }
private:
  inline static QPointer<QJSEngine> s_engine;
};
