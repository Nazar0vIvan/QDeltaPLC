#include "occviewport.h"

#include <AIS_InteractiveContext.hxx>
#include <QDebug>
#include <Standard_Failure.hxx>

#include <functional>

namespace RoboCrap3D {

OccViewport::OccViewport(Aspect_Handle handle, const DrawingDocument& document)
  : m_viewer(handle),
    m_scene(m_viewer.context(), m_viewer.view()),
    m_robot(m_scene),
    m_application(m_scene, document.models),
    m_input(m_viewer.context(), m_viewer.view())
{
  m_timer.setSingleShot(true);
  m_timer.setInterval(0);
  QObject::connect(&m_timer, &QTimer::timeout, &m_timer, std::bind(&OccViewport::flushRender, this));
  if (!m_viewer.isValid()) return;
  m_scene.displayInfrastructure();
  for (const auto& object : document.objects) {
    const auto result = m_application.insertObject(object);
    if (!result.accepted) qWarning().noquote() << "Object drawing replay:" << result.error;
  }
  fitInitialCamera();
}

ViewportOperationResult OccViewport::setRobot(const RobotVisualModel& visuals,
                                              const CadLoadResult& shapes,
                                              const DrawingPose& pose)
{
  if (!isValid()) return {false, QStringLiteral("Robot installation: viewport is unavailable.")};
  const auto result = m_robot.load(visuals, shapes, pose);
  if (result.accepted) fitInitialCamera();
  // Draw now rather than on the next event-loop turn, so the robot is on screen
  // when readiness (which hides the loading overlay) is reported after this call.
  flushRender();
  return result;
}

bool OccViewport::setActiveTool(RobotTool active)
{
  if (!isValid()) return false;
  const bool success = m_robot.setActiveTool(active);
  requestRender();
  return success;
}

void OccViewport::removeObject(quint32 id)
{
  if (m_application.removeObject(id)) requestRender();
}

ViewportOperationResult OccViewport::insertObject(const DrawingObject& object)
{
  const auto result = m_application.insertObject(object);
  if (result.accepted && !m_robot.isLoaded()) fitInitialCamera();
  requestRender();
  return result;
}

ViewportOperationResult OccViewport::setObjectVisible(quint32 id, bool visible)
{
  const auto result = m_application.setObjectVisible(id, visible);
  requestRender();
  return result;
}

ViewportOperationResult OccViewport::setObjectDiagnostics(quint32 id, const DrawingDiagnostics& diagnostics)
{
  const auto result = m_application.setObjectDiagnostics(id, diagnostics);
  requestRender();
  return result;
}

void OccViewport::setSelectedObjects(const QList<quint32>& ids)
{
  m_application.setSelectedObjects(ids);
  requestRender();
}

ViewportOperationResult OccViewport::setDraftAxis(const std::optional<DrawingAxis>& axis)
{
  const auto result = m_application.setDraftAxis(axis);
  requestRender();
  return result;
}

OccViewport::~OccViewport()
{
  m_timer.stop();
}

bool OccViewport::isValid() const
{
  return m_viewer.isValid();
}

void OccViewport::setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet)
{
  m_robot.setSpindleFrames(tcp, collet);
  requestRender();
}

ViewportOperationResult OccViewport::applyTransforms(const DrawingPose& pose)
{
  if (!isValid()) return {false, QStringLiteral("Viewport is unavailable.")};
  const auto result = m_robot.applyTransforms(pose);
  if (result.accepted) requestRender();
  return result;
}

void OccViewport::setExposed(bool exposed)
{
  m_exposed = exposed;
  if (exposed) {
    resize();
  } else {
    m_timer.stop();
    cancelGesture();
  }
}

void OccViewport::resize()
{
  if (!m_exposed || !isValid()) return;
  m_viewer.resize();
  m_scene.updateCameraDependentObjects();
  requestRender();
}

std::optional<quint32> OccViewport::mousePress(const QPoint& pos, Qt::MouseButton button)
{
  if (!m_exposed) return std::nullopt;
  const OccInputResult input = m_input.mousePress(pos, button);
  if (input.accepted && (button == Qt::RightButton || button == Qt::MiddleButton
                         || input.cameraScaleChanged)) m_cameraAdjusted = true;
  applyInput(input);
  if (button != Qt::LeftButton || !input.selectionChanged) return std::nullopt;
  if (m_viewer.context()->HasDetected()) {
    return m_application.objectIdFor(m_viewer.context()->DetectedInteractive());
  }
  return quint32{0}; // Background or non-application geometry clears UI selection.
}

void OccViewport::mouseMove(const QPoint& pos)
{
  if (m_exposed) applyInput(m_input.mouseMove(pos));
}

void OccViewport::mouseRelease(Qt::MouseButton button)
{
  applyInput(m_input.mouseRelease(button));
}

void OccViewport::wheel(const QPoint& pos, int delta)
{
  if (!m_exposed) return;
  const OccInputResult input = m_input.wheel(pos, delta);
  if (input.accepted) m_cameraAdjusted = true;
  applyInput(input);
}

void OccViewport::cancelGesture()
{
  m_input.cancelGesture();
}

void OccViewport::applyInput(const OccInputResult& input)
{
  if (!isValid() || !input.accepted) return;
  if (input.cameraScaleChanged) m_scene.updateCameraDependentObjects();
  if (input.needsRedraw) requestRender();
}

void OccViewport::fitInitialCamera()
{
  if (!m_scene.hasVisibleParts()) return;
  if (!m_cameraAdjusted) m_viewer.fitAll();
  m_scene.updateCameraDependentObjects();
}

void OccViewport::requestRender()
{
  if (m_exposed && isValid() && !m_timer.isActive()) m_timer.start();
}

void OccViewport::flushRender()
{
  if (!m_exposed || !isValid()) return;
  try {
    // UpdateCurrentViewer redraws the viewer; do not immediately redraw it twice.
    m_viewer.updateCurrentViewer();
  } catch (const Standard_Failure& failure) {
    qWarning() << "OCCT redraw failed:" << failure.what();
  }
}

} // namespace RoboCrap3D
