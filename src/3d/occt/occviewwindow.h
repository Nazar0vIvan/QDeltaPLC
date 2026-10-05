#pragma once

#include "3d/robot/robotpreviewstate.h"
#include "cadloadresult.h"
#include "scene/sceneendeffectors.h"
#include "pathgeneration/chamfer/chamferpath.h"

#include <QPointer>
#include <QList>
#include <QWindow>
#include <memory>

class SceneModel;
class SceneObject;

namespace RoboCrap3D {

class OccViewport;

class OccViewWindow final : public QWindow
{
  Q_OBJECT

public:
  OccViewWindow();
  ~OccViewWindow() override;

  void setScene(std::shared_ptr<const RobotPreviewState> state, std::shared_ptr<const CadLoadResult> shapes);
  void setApplicationScene(SceneModel* scene);
  void synchronizeApplicationScene();
  void synchronizeSceneObject(SceneObject* object);
  void removeSceneObject(quint32 objectId);
  void setSelectedObjects(const QList<quint32>& ids);
  void setMachiningPreview(const std::optional<ChamferPathParameters>& parameters);
  void setDiagnosticOverlays(bool showPoints, bool showNormals);
  bool applyPose(const RobotPose& pose);
  // Applied flange-relative calibration; empty hides the TCP decoration.
  void setSpindleTcpFrame(const std::optional<M4d>& frame);
  void setEndEffectors(const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes,
                      SceneEndEffectors::Tool active);
  bool isReady() const;
  bool isViewportReady() const;
  void releaseSurface();

signals:
  void readyChanged();
  void viewportReadyChanged();
  void errorOccurred(const QString& error);
  void applicationSelectionRequested(quint32 objectId, bool additive);
  void deleteSelectionRequested();

protected:
  bool event(QEvent* event) override;
  void exposeEvent(QExposeEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private:
  void initializeViewport();
  void attachRobot();
  void attachEndEffectors();
  void resizeViewport();
  QPoint nativePosition(const QPointF& position) const;

  std::shared_ptr<const RobotPreviewState> m_state;
  std::shared_ptr<const CadLoadResult> m_shapes;
  std::array<std::shared_ptr<const CadLoadResult>, 2> m_toolShapes;
  SceneEndEffectors::Tool m_activeTool = SceneEndEffectors::Measuring;
  QPointer<SceneModel> m_applicationScene;
  std::unique_ptr<OccViewport> m_viewport;
  bool m_initializationFailed = false;
  std::optional<ChamferPathParameters> m_machiningPreview;
  std::optional<M4d> m_spindleTcpFrame;
  QList<quint32> m_selectedObjects;
};

} // namespace RoboCrap3D
