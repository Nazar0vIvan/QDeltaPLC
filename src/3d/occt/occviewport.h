#pragma once

#include "occinputcontroller.h"
#include "occviewer.h"
#include "3d/robot/robotpreviewstate.h"
#include "3d/robot/visualization/robotoccsceneadapter.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <optional>

class SceneModel;
class SceneObject;

namespace RoboCrap3D {

class OccViewport final
{
public:
  OccViewport(Aspect_Handle handle, const RobotPreviewState& state, const CadLoadResult& shapes,
              SceneModel* applicationScene);
  ~OccViewport();
  OccViewport(const OccViewport&) = delete;
  OccViewport& operator=(const OccViewport&) = delete;

  bool isValid() const;
  bool applyPose(const RobotPose& pose);
  void synchronizeApplicationScene(SceneModel* applicationScene);
  void setSelectedObjects(const QStringList& ids);
  void setDiagnosticOverlays(bool showPoints, bool showNormals, SceneModel* applicationScene);
  void setExposed(bool exposed);
  void resize();
  std::optional<QString> mousePress(const QPoint& pos, Qt::MouseButton button);
  void mouseMove(const QPoint& pos);
  void mouseRelease(Qt::MouseButton button);
  void wheel(const QPoint& pos, int delta);
  void cancelGesture();

private:
  void applyInput(const OccInputResult& input);
  void requestRender();
  void flushRender();
  bool synchronizeOverlay(const SceneObject* object, std::optional<OccScene::PartId>& partId,
                          bool enabled, bool normals);

  struct OverlayParts {
    std::optional<OccScene::PartId> points;
    std::optional<OccScene::PartId> normals;
  };

  OccViewer m_viewer;
  OccScene m_scene;
  RobotOccSceneAdapter m_robot;
  OccInputController m_input;
  QTimer m_timer;
  // Derived viewport handles only; application data remains owned by SceneModel.
  QHash<QString, OccScene::PartId> m_applicationParts;
  QHash<QString, OverlayParts> m_applicationOverlays;
  bool m_showPoints = false;
  bool m_showNormals = false;
  bool m_ready = false;
  bool m_exposed = false;
};

} // namespace RoboCrap3D
