#include "occviewwindow.h"

#include "occviewport.h"
#include "scene/scenemodel.h"

#include <QExposeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QWheelEvent>

#include <Standard_Failure.hxx>

#include <cmath>
#include <utility>

namespace RoboCrap3D {

OccViewWindow::OccViewWindow()
{
  setSurfaceType(QSurface::OpenGLSurface);
  QObject::connect(this, &QWindow::screenChanged, this, &OccViewWindow::resizeViewport);
}

void OccViewWindow::resizeViewport()
{
  if (m_viewport) m_viewport->resize();
}

OccViewWindow::~OccViewWindow()
{
  m_viewport.reset();
}

void OccViewWindow::setScene(std::shared_ptr<const RobotPreviewState> state,
                             std::shared_ptr<const CadLoadResult> shapes)
{
  m_state = std::move(state);
  m_shapes = std::move(shapes);
  m_initializationFailed = false;
  if (m_viewport)
    attachRobot();
  else if (isExposed())
    initializeViewport();
}

void OccViewWindow::setApplicationScene(SceneModel* scene)
{
  m_applicationScene = scene;
  synchronizeApplicationScene();
}

void OccViewWindow::synchronizeApplicationScene()
{
  if (m_viewport) {
    m_viewport->synchronizeApplicationScene(m_applicationScene);
    m_viewport->setSelectedObjects(m_selectedObjects);
  }
}

void OccViewWindow::removeSceneObject(quint32 objectId)
{
  if (m_viewport) m_viewport->removeSceneObject(objectId);
}

void OccViewWindow::synchronizeSceneObject(SceneObject* object)
{
  if (m_viewport) {
    m_viewport->synchronizeSceneObject(object);
    m_viewport->setSelectedObjects(m_selectedObjects);
  }
}

void OccViewWindow::setSelectedObjects(const QList<quint32>& ids)
{
  m_selectedObjects = ids;
  if (m_viewport) m_viewport->setSelectedObjects(ids);
}

void OccViewWindow::setMachiningPathsVisible(bool visible)
{
  m_showMachiningPaths = visible;
  if (m_viewport) {
    m_viewport->setMachiningPathsVisible(visible, m_applicationScene);
    m_viewport->setSelectedObjects(m_selectedObjects);
  }
}

void OccViewWindow::setMachiningFrames(const QVector<SceneCoordinateFrame>& frames)
{
  m_machiningFrames = frames;
  if (m_viewport) m_viewport->setMachiningFrames(frames);
}

void OccViewWindow::setMachiningPreview(const std::optional<ChamferPathParameters>& parameters)
{
  m_machiningPreview = parameters;
  if (m_viewport) m_viewport->setMachiningPreview(parameters);
}

void OccViewWindow::setDiagnosticOverlays(bool showPoints, bool showNormals)
{
  if (m_viewport) m_viewport->setDiagnosticOverlays(showPoints, showNormals, m_applicationScene);
}

void OccViewWindow::setSpindleTcpFrame(const std::optional<M4d>& frame)
{
  m_spindleTcpFrame = frame;
  if (m_viewport) m_viewport->setSpindleTcpFrame(frame);
}

bool OccViewWindow::applyPose(const RobotPose& pose)
{
  return isReady() && m_viewport->applyPose(pose);
}

void OccViewWindow::setEndEffectors(
    const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes, SceneEndEffectors::Tool active)
{
  m_toolShapes = shapes;
  m_activeTool = active;
  attachEndEffectors();
}

void OccViewWindow::attachEndEffectors()
{
  if (m_viewport && !m_viewport->setEndEffectors(m_toolShapes, m_activeTool))
    emit errorOccurred(QStringLiteral("Cannot update the end-effector presentation."));
}

bool OccViewWindow::isReady() const
{
  return m_viewport && m_viewport->isRobotReady();
}

bool OccViewWindow::isViewportReady() const
{
  return m_viewport && m_viewport->isValid();
}

void OccViewWindow::releaseSurface()
{
  const bool wasReady = isReady();
  const bool wasViewportReady = isViewportReady();
  m_viewport.reset();
  if (wasReady) emit readyChanged();
  if (wasViewportReady) emit viewportReadyChanged();
}

bool OccViewWindow::event(QEvent* event)
{
  try {
    if (event->type() == QEvent::PlatformSurface) {
      const auto* surface = static_cast<QPlatformSurfaceEvent*>(event);
      if (surface->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
        releaseSurface();
        m_initializationFailed = false;
      }
    } else if (event->type() == QEvent::DevicePixelRatioChange) {
      if (m_viewport) m_viewport->resize();
    } else if (event->type() == QEvent::FocusOut || event->type() == QEvent::UngrabMouse
               || event->type() == QEvent::Hide) {
      if (m_viewport) {
        m_viewport->cancelGesture();
        if (event->type() == QEvent::Hide) m_viewport->setExposed(false);
      }
    }
    return QWindow::event(event);
  } catch (const Standard_Failure& failure) {
    releaseSurface();
    m_initializationFailed = true;
    emit errorOccurred(QStringLiteral("Viewport operation failed: %1")
                           .arg(QString::fromUtf8(failure.what())));
    return true;
  }
}

void OccViewWindow::exposeEvent(QExposeEvent*)
{
  if (isExposed()) initializeViewport();
  if (m_viewport) m_viewport->setExposed(isExposed());
}

void OccViewWindow::resizeEvent(QResizeEvent*)
{
  if (m_viewport) m_viewport->resize();
}

void OccViewWindow::mousePressEvent(QMouseEvent* event)
{
  if (!isViewportReady()) return;
  const auto picked = m_viewport->mousePress(nativePosition(event->position()), event->button());
  if (picked) emit applicationSelectionRequested(*picked,
                                                  (event->modifiers() & Qt::ControlModifier) != 0);
  event->accept();
}

void OccViewWindow::mouseMoveEvent(QMouseEvent* event)
{
  if (!isViewportReady()) return;
  if (event->buttons() == Qt::NoButton) m_viewport->cancelGesture();
  m_viewport->mouseMove(nativePosition(event->position()));
  event->accept();
}

void OccViewWindow::mouseReleaseEvent(QMouseEvent* event)
{
  if (!isViewportReady()) return;
  m_viewport->mouseRelease(event->button());
  if (parent()) parent()->requestActivate();
  event->accept();
}

void OccViewWindow::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete && event->modifiers() == Qt::NoModifier) {
    if (!event->isAutoRepeat()) emit deleteSelectionRequested();
    event->accept();
    return;
  }
  QWindow::keyPressEvent(event);
}

void OccViewWindow::wheelEvent(QWheelEvent* event)
{
  if (!isViewportReady()) return;
  m_viewport->wheel(nativePosition(event->position()), event->angleDelta().y());
  event->accept();
}

void OccViewWindow::initializeViewport()
{
  if (m_viewport || m_initializationFailed || !isExposed()) return;
  try {
    auto viewport = std::make_unique<OccViewport>(reinterpret_cast<Aspect_Handle>(winId()),
                                                  m_applicationScene, m_showMachiningPaths);
    if (!viewport->isValid()) {
      m_initializationFailed = true;
      emit errorOccurred(QStringLiteral("Cannot create the OCCT viewport."));
      return;
    }
    m_viewport = std::move(viewport);
    m_viewport->setMachiningPathsVisible(m_showMachiningPaths, m_applicationScene);
    m_viewport->setMachiningPreview(m_machiningPreview);
    m_viewport->setMachiningFrames(m_machiningFrames);
    m_viewport->setSpindleTcpFrame(m_spindleTcpFrame);
    m_viewport->setSelectedObjects(m_selectedObjects);
    m_viewport->setExposed(true);
  } catch (const Standard_Failure& failure) {
    releaseSurface();
    m_initializationFailed = true;
    emit errorOccurred(QStringLiteral("Cannot initialize the viewport: %1")
                           .arg(QString::fromUtf8(failure.what())));
    return;
  }
  // Robot failures must not enter the viewport teardown path above.
  attachRobot();
  emit viewportReadyChanged();
}

void OccViewWindow::attachRobot()
{
  if (!isViewportReady()) return;
  const bool wasReady = isReady();
  try {
    if (!m_viewport->setRobot(m_state.get(), m_shapes.get()))
      emit errorOccurred(QStringLiteral("Cannot create the OCCT robot presentation."));
  } catch (const Standard_Failure& failure) {
    emit errorOccurred(QStringLiteral("Cannot attach the robot presentation: %1")
                           .arg(QString::fromUtf8(failure.what())));
  }
  attachEndEffectors();
  if (wasReady != isReady()) emit readyChanged();
}

QPoint OccViewWindow::nativePosition(const QPointF& position) const
{
  return {static_cast<int>(std::lround(position.x() * devicePixelRatio())),
          static_cast<int>(std::lround(position.y() * devicePixelRatio()))};
}

} // namespace RoboCrap3D
