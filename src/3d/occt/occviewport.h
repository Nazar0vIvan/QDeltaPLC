#pragma once

#include "occinputcontroller.h"
#include "occviewer.h"
#include "3d/robot/robotpreviewstate.h"
#include "3d/adapters/occrobotadapter.h"
#include "3d/adapters/occsceneadapter.h"

#include <QString>
#include <QList>
#include <QTimer>

#include <optional>

class SceneModel;
class SceneObject;

namespace RoboCrap3D {

class OccViewport final
{
public:
  // Install the scene before initial camera fitting.
  OccViewport(Aspect_Handle handle, SceneModel* applicationScene);
  ~OccViewport();
  OccViewport(const OccViewport&) = delete;
  OccViewport& operator=(const OccViewport&) = delete;

  bool isValid() const;
  bool isRobotReady() const { return isValid() && m_robot.isLoaded(); }
  bool setRobot(const RobotPreviewState* state, const CadLoadResult* shapes);
  bool applyPose(const RobotPose& pose);
  // Applied flange-relative calibration; empty hides the TCP decoration.
  void setSpindleTcpFrame(const std::optional<M4d>& frame);
  bool setEndEffectors(const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes,
                      SceneEndEffectors::Tool active);
  void synchronizeApplicationScene(SceneModel* applicationScene);
  void synchronizeSceneObject(SceneObject* object);
  void removeSceneObject(quint32 objectId);
  void setSelectedObjects(const QList<quint32>& ids);
  void setMachiningPreview(const std::optional<ChamferPathParameters>& parameters);
  void setDiagnosticOverlays(bool showPoints, bool showNormals, SceneModel* applicationScene);
  void setExposed(bool exposed);
  void resize();
  std::optional<quint32> mousePress(const QPoint& pos, Qt::MouseButton button);
  void mouseMove(const QPoint& pos);
  void mouseRelease(Qt::MouseButton button);
  void wheel(const QPoint& pos, int delta);
  void cancelGesture();

private:
  void applyInput(const OccInputResult& input);
  void fitInitialCamera();
  void requestRender();
  void flushRender();

  OccViewer m_viewer;
  OccScene m_scene;
  OccRobotAdapter m_robot;
  OccSceneAdapter m_application;
  OccInputController m_input;
  QTimer m_timer;
  bool m_cameraAdjusted = false;
  bool m_exposed = false;
};

} // namespace RoboCrap3D
