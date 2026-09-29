#include "occviewport.h"

#include <AIS_InteractiveContext.hxx>
#include <QDebug>
#include <Standard_Failure.hxx>

#include <functional>

namespace RoboCrap3D {

OccViewport::OccViewport(Aspect_Handle handle, SceneModel* applicationScene, bool showMachiningPaths)
  : m_viewer(handle),
    m_scene(m_viewer.context(), m_viewer.view()),
    m_robot(m_scene),
    m_application(m_scene),
    m_input(m_viewer.context(), m_viewer.view())
{
  m_timer.setSingleShot(true);
  m_timer.setInterval(0);
  QObject::connect(&m_timer, &QTimer::timeout, &m_timer, std::bind(&OccViewport::flushRender, this));
  if (!m_viewer.isValid()) return;
  m_scene.displayInfrastructure();
  (void)m_application.setMachiningPathsVisible(showMachiningPaths, nullptr);
  (void)m_application.synchronize(applicationScene);
  fitInitialCamera();
}

bool OccViewport::setRobot(const RobotPreviewState* state, const CadLoadResult* shapes)
{
  if (!isValid()) return false;
  if (!state || !shapes) {
    m_robot.clear();
    requestRender();
    return true;
  }
  const bool loaded = m_robot.load(state->model().visuals, *shapes, state->pose().transforms());
  requestRender();
  if (loaded) fitInitialCamera();
  return loaded;
}

void OccViewport::synchronizeApplicationScene(SceneModel* applicationScene)
{
  if (m_application.synchronize(applicationScene)) {
    if (!m_robot.isLoaded()) fitInitialCamera();
    requestRender();
  }
}

bool OccViewport::setEndEffectors(
    const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes, SceneEndEffectors::Tool active)
{
  if (!isValid()) return false;
  const bool success = m_robot.setEndEffectors(shapes, active);
  requestRender();
  return success;
}

void OccViewport::removeSceneObject(quint32 objectId)
{
  if (m_application.removeObject(objectId)) requestRender();
}

void OccViewport::synchronizeSceneObject(SceneObject* object)
{
  if (m_application.synchronizeObject(object)) {
    if (!m_robot.isLoaded()) fitInitialCamera();
    requestRender();
  }
}

void OccViewport::setDiagnosticOverlays(bool showPoints, bool showNormals,
                                        SceneModel* applicationScene)
{
  if (m_application.setDiagnosticOverlays(showPoints, showNormals, applicationScene)) requestRender();
}

void OccViewport::setSelectedObjects(const QList<quint32>& ids)
{
  m_application.setSelectedObjects(ids);
  requestRender();
}

void OccViewport::setMachiningPathsVisible(bool visible, SceneModel* scene)
{
  if (m_application.setMachiningPathsVisible(visible, scene)) requestRender();
}

void OccViewport::setMachiningFrames(const QVector<SceneCoordinateFrame>& frames)
{
  m_application.setMachiningFrames(frames);
  requestRender();
}

void OccViewport::setMachiningPreview(const std::optional<ChamferPathParameters>& parameters)
{
  if (m_application.setMachiningPreview(parameters)) requestRender();
}

OccViewport::~OccViewport()
{
  m_timer.stop();
}

bool OccViewport::isValid() const
{
  return m_viewer.isValid();
}

void OccViewport::setSpindleTcpFrame(const std::optional<M4d>& frame)
{
  m_robot.setSpindleTcpFrame(frame);
  requestRender();
}

bool OccViewport::applyPose(const RobotPose& pose)
{
  if (!isValid() || !m_robot.applyTransforms(pose.transforms())) return false;
  requestRender();
  return true;
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
