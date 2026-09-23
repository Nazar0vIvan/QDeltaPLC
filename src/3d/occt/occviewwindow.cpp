#include "occviewwindow.h"

#include "occviewport.h"
#include "scene/scenemodel.h"

#include <QExposeEvent>
#include <QMouseEvent>
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

void OccViewWindow::setScene(std::shared_ptr<RobotPreviewState> state,
                             std::shared_ptr<const CadLoadResult> shapes)
{
  releaseSurface();
  m_state = std::move(state);
  m_shapes = std::move(shapes);
  m_initializationFailed = false;
  if (isExposed()) initializeViewport();
}

void OccViewWindow::setApplicationScene(SceneModel* scene)
{
  m_applicationScene = scene;
  synchronizeApplicationScene();
}

void OccViewWindow::synchronizeApplicationScene()
{
  if (m_viewport) m_viewport->synchronizeApplicationScene(m_applicationScene);
}

void OccViewWindow::setSelectedObjects(const QStringList& ids)
{
  if (m_viewport) m_viewport->setSelectedObjects(ids);
}

void OccViewWindow::setDiagnosticOverlays(bool showPoints, bool showNormals)
{
  if (m_viewport) m_viewport->setDiagnosticOverlays(showPoints, showNormals, m_applicationScene);
}

bool OccViewWindow::applyPose(const RobotPose& pose)
{
  return isReady() && m_viewport->applyPose(pose);
}

bool OccViewWindow::isReady() const
{
  return m_viewport && m_viewport->isValid();
}

void OccViewWindow::releaseSurface()
{
  const bool wasReady = isReady();
  m_viewport.reset();
  if (wasReady) emit readyChanged();
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
  if (!isReady()) return;
  const auto picked = m_viewport->mousePress(nativePosition(event->position()), event->button());
  if (picked) emit applicationSelectionRequested(*picked,
                                                  (event->modifiers() & Qt::ControlModifier) != 0);
  event->accept();
}

void OccViewWindow::mouseMoveEvent(QMouseEvent* event)
{
  if (!isReady()) return;
  if (event->buttons() == Qt::NoButton) m_viewport->cancelGesture();
  m_viewport->mouseMove(nativePosition(event->position()));
  event->accept();
}

void OccViewWindow::mouseReleaseEvent(QMouseEvent* event)
{
  if (!isReady()) return;
  m_viewport->mouseRelease(event->button());
  if (parent()) parent()->requestActivate();
  event->accept();
}

void OccViewWindow::wheelEvent(QWheelEvent* event)
{
  if (!isReady()) return;
  m_viewport->wheel(nativePosition(event->position()), event->angleDelta().y());
  event->accept();
}

void OccViewWindow::initializeViewport()
{
  if (m_viewport || m_initializationFailed || !m_state || !m_shapes || !isExposed()) return;
  try {
    auto viewport = std::make_unique<OccViewport>(reinterpret_cast<Aspect_Handle>(winId()),
                                                  *m_state, *m_shapes, m_applicationScene);
    if (!viewport->isValid()) {
      m_initializationFailed = true;
      emit errorOccurred(QStringLiteral("Cannot create the OCCT robot presentation."));
      return;
    }
    m_viewport = std::move(viewport);
    m_viewport->setExposed(true);
    emit readyChanged();
  } catch (const Standard_Failure& failure) {
    releaseSurface();
    m_initializationFailed = true;
    emit errorOccurred(QStringLiteral("Cannot initialize the viewport: %1")
                           .arg(QString::fromUtf8(failure.what())));
  }
}

QPoint OccViewWindow::nativePosition(const QPointF& position) const
{
  return {static_cast<int>(std::lround(position.x() * devicePixelRatio())),
          static_cast<int>(std::lround(position.y() * devicePixelRatio()))};
}

} // namespace RoboCrap3D
