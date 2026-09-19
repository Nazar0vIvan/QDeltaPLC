#include "occviewport.h"

#include <QDebug>
#include <Standard_Failure.hxx>

namespace RoboCrap3D {

OccViewport::OccViewport(Aspect_Handle handle, const RobotPreviewState& state, const CadLoadResult& shapes)
  : m_viewer(handle),
    m_scene(m_viewer.context(), m_viewer.view()),
    m_robot(m_scene),
    m_input(m_viewer.context(), m_viewer.view())
{
  m_timer.setSingleShot(true);
  m_timer.setInterval(0);
  QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this]() { flushRender(); });
  if (!m_viewer.isValid()) return;
  m_scene.displayInfrastructure();
  if (!m_robot.load(state.model(), shapes) || !m_robot.applyTransforms(state.pose().transforms)) return;
  m_viewer.fitAll();
  m_scene.updateCameraDependentObjects();
  m_ready = true;
}

OccViewport::~OccViewport()
{
  m_timer.stop();
}

bool OccViewport::isValid() const
{
  return m_ready && m_viewer.isValid();
}

bool OccViewport::applyPose(const RobotPose& pose)
{
  if (!isValid() || !m_robot.applyTransforms(pose.transforms)) return false;
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

void OccViewport::mousePress(const QPoint& pos, Qt::MouseButton button)
{
  if (m_exposed) applyInput(m_input.mousePress(pos, button));
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
  if (m_exposed) applyInput(m_input.wheel(pos, delta));
}

void OccViewport::cancelGesture()
{
  m_input.cancelGesture();
}

void OccViewport::applyInput(const OccInputResult& input)
{
  if (!isValid() || !input.accepted) return;
  if (input.cameraScaleChanged) m_scene.updateCameraDependentObjects();
  if (input.needsRedraw || input.needsViewerUpdate) requestRender();
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
