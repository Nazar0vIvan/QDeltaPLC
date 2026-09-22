#include "occinputcontroller.h"

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewCube.hxx>

#include <V3d_View.hxx>

#include <cmath>

namespace RoboCrap3D {

OccInputController::OccInputController(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view) : m_context(context), m_view(view)
{}

bool OccInputController::isValid() const
{
  return !m_context.IsNull() && !m_view.IsNull();
}

void OccInputController::cancelGesture()
{
  m_rotating = false;
  m_panning = false;
}

OccInputResult OccInputController::mousePress(const QPoint& pos, const Qt::MouseButton button)
{
  if (!isValid()) return {};

  m_lastMousePos = pos;

  switch (button) {
    case Qt::LeftButton: return handleLeftButtonPress(pos);
    case Qt::RightButton: return handleRightButtonPress(pos);
    case Qt::MiddleButton: return handleMiddleButtonPress(pos);
    default:  return {};
  }
}

OccInputResult OccInputController::mouseMove(const QPoint& pos)
{
  if (!isValid()) return {};

  if (m_rotating) return handleRotationMove(pos);

  if (m_panning) return handlePanMove(pos);

  return handleHoverMove(pos);
}

OccInputResult OccInputController::mouseRelease(const Qt::MouseButton button)
{
  if (!isValid()) return {};

  if (button == Qt::RightButton) {
    m_rotating = false;

    OccInputResult result;
    result.accepted = true;
    return result;
  }

  if (button == Qt::MiddleButton) {
    m_panning = false;

    OccInputResult result;
    result.accepted = true;
    return result;
  }

  return {};
}

OccInputResult OccInputController::wheel(const QPoint& pos, const int angleDeltaY)
{
  if (!isValid()) return {};
  if (angleDeltaY == 0) return {};

  // One standard wheel notch = 120 units. Scale pixel offset proportionally.
  constexpr double kNotch = 120.0;
  constexpr double kPixelsPerNotch = 10.0;
  const int delta = static_cast<int>(std::round((angleDeltaY / kNotch) * kPixelsPerNotch));

  m_view->Zoom(pos.x(), pos.y(), pos.x() + delta, pos.y() + delta);

  OccInputResult result;
  result.accepted = true;
  result.needsRedraw = true;
  result.needsViewerUpdate = true;
  result.cameraChanged = true;
  result.cameraScaleChanged = true;
  return result;
}

OccInputResult OccInputController::handleLeftButtonPress(const QPoint& pos)
{
  m_context->MoveTo(pos.x(), pos.y(), m_view, false);
  if (!m_context->HasDetected()) return clearSelection();

  const Handle(AIS_ViewCubeOwner) owner = Handle(AIS_ViewCubeOwner)::DownCast(m_context->DetectedOwner());

  if (!owner.IsNull()) return handleCube(owner);

  return handleDetectedSelectable();
}

OccInputResult OccInputController::handleRightButtonPress(const QPoint& pos)
{
  m_rotating = true;
  m_panning = false;

  m_view->StartRotation(pos.x(), pos.y());

  OccInputResult result;
  result.accepted = true;

  return result;
}

OccInputResult OccInputController::handleMiddleButtonPress(const QPoint& pos)
{
  Q_UNUSED(pos);

  m_panning = true;
  m_rotating = false;

  OccInputResult result;
  result.accepted = true;

  return result;
}

OccInputResult OccInputController::handleRotationMove(const QPoint& pos)
{
  m_view->Rotation(pos.x(), pos.y());
  m_lastMousePos = pos;  // keep position current so pan-after-rotate has no jump

  OccInputResult result;
  result.accepted      = true;
  result.needsRedraw   = true;
  result.cameraChanged = true;
  return result;
}

OccInputResult OccInputController::handlePanMove(const QPoint& pos)
{
  const QPoint delta = pos - m_lastMousePos;

  m_view->Pan(delta.x(), -delta.y());

  m_lastMousePos = pos;

  OccInputResult result;
  result.accepted = true;
  result.needsRedraw = true;
  result.cameraChanged = true;

  return result;
}

OccInputResult OccInputController::handleHoverMove(const QPoint& pos)
{
  m_context->MoveTo(pos.x(), pos.y(), m_view, false);

  OccInputResult input;
  input.accepted = true;
  input.needsViewerUpdate = true;
  input.hoverChanged = true;
  return input;
}

OccInputResult OccInputController::handleCube(const Handle(AIS_ViewCubeOwner)& owner)
{
  const Handle(AIS_ViewCube) cube = Handle(AIS_ViewCube)::DownCast(owner->Selectable());

  if (cube.IsNull()) return {};

  cube->HandleClick(owner);

  OccInputResult input;
  input.accepted = true;
  input.needsRedraw = true;
  input.needsViewerUpdate = true;
  input.cameraChanged = true;
  input.cameraScaleChanged = true;
  return input;
}

OccInputResult OccInputController::handleDetectedSelectable()
{
  OccInputResult result;
  result.accepted          = true;
  result.needsRedraw       = true;
  result.needsViewerUpdate = true;
  result.selectionChanged  = true;

  return result;
}

OccInputResult OccInputController::clearSelection()
{
  m_context->ClearSelected(false);

  OccInputResult result;
  result.accepted = true;
  result.needsRedraw = true;
  result.needsViewerUpdate = true;
  result.selectionChanged = true;

  return result;
}

} // namespace RoboCrap3D
