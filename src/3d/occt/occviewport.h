#pragma once

#include "occinputcontroller.h"
#include "occviewer.h"
#include "3d/robot/robotpreviewstate.h"
#include "3d/robot/visualization/robotoccsceneadapter.h"

#include <QTimer>

namespace RoboCrap3D {

class OccViewport final
{
public:
  OccViewport(Aspect_Handle handle, const RobotPreviewState& state, const CadLoadResult& shapes);
  ~OccViewport();
  OccViewport(const OccViewport&) = delete;
  OccViewport& operator=(const OccViewport&) = delete;

  bool isValid() const;
  bool applyPose(const RobotPose& pose);
  void setExposed(bool exposed);
  void resize();
  void mousePress(const QPoint& pos, Qt::MouseButton button);
  void mouseMove(const QPoint& pos);
  void mouseRelease(Qt::MouseButton button);
  void wheel(const QPoint& pos, int delta);
  void cancelGesture();

private:
  void applyInput(const OccInputResult& input);
  void requestRender();
  void flushRender();

  OccViewer m_viewer;
  OccScene m_scene;
  RobotOccSceneAdapter m_robot;
  OccInputController m_input;
  QTimer m_timer;
  bool m_ready = false;
  bool m_exposed = false;
};

} // namespace RoboCrap3D
