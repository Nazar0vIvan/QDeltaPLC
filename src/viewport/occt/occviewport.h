#pragma once

#include "occinputcontroller.h"
#include "occviewer.h"
#include "viewport/occcontroller.h"
#include "adapters/occrobotadapter.h"
#include "adapters/occsceneadapter.h"

#include <QString>
#include <QList>
#include <QTimer>

#include <optional>

namespace RoboCrap3D {

class OccViewport final
{
public:
  OccViewport(Aspect_Handle handle, const DrawingDocument& document);
  ~OccViewport();
  OccViewport(const OccViewport&) = delete;
  OccViewport& operator=(const OccViewport&) = delete;

  bool isValid() const;
  bool isRobotReady() const { return isValid() && m_robot.isLoaded(); }
  // Ordered graphics, loaded shapes and transforms define the installation.
  ViewportOperationResult setRobot(const RobotVisualModel& visuals, const CadLoadResult& shapes,
                const DrawingPose& pose);
  ViewportOperationResult applyTransforms(const DrawingPose& pose);
  // BASE-relative drawing frames; absent values hide the corresponding axes.
  void setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet);
  bool setEndEffectors(const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes,
                      RobotTool active);
  ViewportOperationResult insertObject(const DrawingObject& object);
  void removeObject(quint32 id);
  ViewportOperationResult setObjectVisible(quint32 id, bool visible);
  ViewportOperationResult setObjectDiagnostics(quint32 id, const DrawingDiagnostics& diagnostics);
  void setSelectedObjects(const QList<quint32>& ids);
  ViewportOperationResult setDraftAxis(const std::optional<DrawingAxis>& axis);
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
