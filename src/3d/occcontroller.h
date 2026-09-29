#pragma once

#include "scene/sceneendeffectors.h"

#include <QObject>
#include <QMetaObject>
#include <QPointer>
#include <QList>
#include <QVariantList>
#include <QWindow>

#include <memory>
#include <array>

class SceneModel;
class SceneObject;
class SceneMachining;

namespace RoboCrap3D {

class CadLoadWorker;
class OccViewWindow;
class RobotPreviewState;
struct CadLoadResult;
class RobotPose;

class OccController final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QWindow* viewWindow READ viewWindow NOTIFY viewWindowChanged)
  Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
  Q_PROPERTY(bool showMachiningPaths READ showMachiningPaths WRITE setShowMachiningPaths NOTIFY showMachiningPathsChanged)
  Q_PROPERTY(bool viewportReady READ isViewportReady NOTIFY viewportReadyChanged)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
  Q_PROPERTY(QString warningString READ warningString NOTIFY warningStringChanged)
  Q_PROPERTY(QVariantList jointAngles READ jointAngles NOTIFY poseChanged)
  Q_PROPERTY(QVariantList flangePose READ flangePose NOTIFY poseChanged)
  Q_PROPERTY(QList<double> tcpPose READ tcpPose NOTIFY tcpPoseChanged)
  Q_PROPERTY(bool measuringCadLoading READ isMeasuringCadLoading NOTIFY endEffectorLoadChanged)
  Q_PROPERTY(bool spindleCadLoading READ isSpindleCadLoading NOTIFY endEffectorLoadChanged)
  Q_PROPERTY(QString measuringCadError READ measuringCadError NOTIFY endEffectorLoadChanged)
  Q_PROPERTY(QString spindleCadError READ spindleCadError NOTIFY endEffectorLoadChanged)

public:
  explicit OccController(QObject* parent = nullptr);
  ~OccController() override;
  Q_DISABLE_COPY_MOVE(OccController)

  QWindow* viewWindow() const;
  bool isReady() const;
  bool isViewportReady() const;
  bool isLoading() const { return m_loading; }
  QString errorString() const { return m_error; }
  QString warningString() const { return m_warning; }
  QVariantList jointAngles() const;
  QVariantList flangePose() const;
  QList<double> tcpPose() const;
  void setApplicationScene(SceneModel* scene);
  void setEndEffectors(SceneEndEffectors* effectors);
  void setMachining(SceneMachining* machining);
  bool showMachiningPaths() const { return m_showMachiningPaths; }
  void setShowMachiningPaths(bool visible);
  bool isMeasuringCadLoading() const;
  bool isSpindleCadLoading() const;
  QString measuringCadError() const;
  QString spindleCadError() const;
  Q_INVOKABLE void reloadEndEffector(SceneEndEffectors::Tool tool);
  Q_INVOKABLE void setSelectedObjects(const QList<quint32>& ids);
  Q_INVOKABLE void setDiagnosticOverlays(bool showPoints, bool showNormals);

  Q_INVOKABLE void loadRobot();
  Q_INVOKABLE void detachViewWindow(QWindow* window);
  Q_INVOKABLE QVariantList solveFK(const QVariantList& joints);
  Q_INVOKABLE QVariantList solveIK(const QVariantList& flange);
  Q_INVOKABLE QVariantList solveTcpIK(const QVariantList& tcp);

signals:
  void showMachiningPathsChanged();
  void viewWindowChanged();
  void readyChanged();
  void viewportReadyChanged();
  void loadingChanged();
  void errorStringChanged();
  void warningStringChanged();
  void poseChanged();
  void tcpPoseChanged();
  void message(const QString& text, bool error);
  void applicationSelectionRequested(quint32 objectId, bool additive);
  void deleteSelectionRequested();
  void endEffectorLoadChanged();
  void endEffectorLoaded(SceneEndEffectors::Tool tool);

private:
  void createWindow();
  void synchronizeMachiningReadiness();
  void synchronizeMachiningPreview();
  void applyPlaybackPose(QList<double> joints);
  void synchronizeEndEffectors();
  void synchronizeSceneObject(SceneObject* object);
  void removeSceneObject(quint32 objectId);
  void onSceneDestroyed();
  void onViewWindowDestroyed();
  void finishRobotLoad(quint64 generation, std::shared_ptr<RobotPreviewState> pending);
  void startEndEffectorLoad(SceneEndEffectors::Tool tool);
  // Tool identity, request generation and source identify an asynchronous result.
  void finishEndEffectorLoad(SceneEndEffectors::Tool tool, quint64 generation, const QUrl& source);
  void setError(const QString& error);
  bool applyPose(const RobotPose& pose);
  QVariantList solve(const QVariantList& values, bool inverse);

  struct ToolCadState {
    QUrl loadedSource;
    std::shared_ptr<const CadLoadResult> shapes;
    std::unique_ptr<CadLoadWorker> worker;
    QString error;
    quint64 generation = 0;
  };

  std::array<ToolCadState, 2> m_toolCad;
  QPointer<SceneEndEffectors> m_endEffectors;
  QPointer<SceneMachining> m_machining;
  std::shared_ptr<RobotPreviewState> m_state;
  std::shared_ptr<const CadLoadResult> m_shapes;
  std::unique_ptr<CadLoadWorker> m_worker;
  QPointer<OccViewWindow> m_window;
  QPointer<SceneModel> m_applicationScene;
  QMetaObject::Connection m_sceneCollectionConnection;
  QMetaObject::Connection m_sceneVisibilityConnection;
  QMetaObject::Connection m_sceneRemovalConnection;
  QMetaObject::Connection m_sceneDestroyedConnection;
  QString m_error;
  QString m_warning;
  quint64 m_generation = 0;
  bool m_loading = false;
  bool m_shuttingDown = false;
  bool m_occtInitialized = false;
  bool m_showMachiningPaths = true;
};

} // namespace RoboCrap3D
