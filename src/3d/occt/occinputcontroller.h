#pragma once

#include <QPoint>
#include <Qt>

#include <Standard_Handle.hxx>

class AIS_ViewCubeOwner;
class AIS_InteractiveContext;
class V3d_View;

namespace RoboCrap3D {

struct OccInputResult
{
  bool accepted = false;
  bool needsRedraw = false;
  bool needsViewerUpdate = false;
  bool cameraChanged = false;
  bool cameraScaleChanged = false;
  bool selectionChanged = false;
  bool hoverChanged = false;
};

class OccInputController final
{
public:
  OccInputController(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view);

  bool isValid() const;

  OccInputResult mousePress(const QPoint& pos, Qt::MouseButton button);
  OccInputResult mouseMove(const QPoint& pos);
  OccInputResult mouseRelease(Qt::MouseButton button);
  OccInputResult wheel(const QPoint& pos, int angleDeltaY);
  void cancelGesture();

private:
  OccInputResult handleLeftButtonPress(const QPoint& pos);
  OccInputResult handleRightButtonPress(const QPoint& pos);
  OccInputResult handleMiddleButtonPress(const QPoint& pos);

  OccInputResult handleRotationMove(const QPoint& pos);
  OccInputResult handlePanMove(const QPoint& pos);
  OccInputResult handleHoverMove(const QPoint& pos);
  OccInputResult handleCube(const Handle(AIS_ViewCubeOwner)& owner);

  OccInputResult handleDetectedSelectable();
  OccInputResult clearSelection();

private:
  Handle(AIS_InteractiveContext) m_context;
  Handle(V3d_View) m_view;

  QPoint m_lastMousePos;

  bool m_rotating = false;
  bool m_panning = false;
};

} // namespace RoboCrap3D
