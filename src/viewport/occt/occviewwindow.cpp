#include "occviewwindow.h"

#include "occviewport.h"

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

OccViewWindow::OccViewWindow(const DrawingDocument& document) : m_document(document)
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

void OccViewWindow::setScene(const RobotVisualModel& visuals,
                              std::shared_ptr<const CadLoadResult> shapes,
                              const DrawingPose& pose)
{
  if (m_shapes) return;
  m_visuals = visuals;
  m_pose = pose;
  m_shapes = std::move(shapes);
  if (m_viewport)
    attachRobot();
  else if (isExposed())
    initializeViewport();
}

ViewportOperationResult OccViewWindow::insertObject(const DrawingObject& object)
{
  const auto result = m_viewport ? m_viewport->insertObject(object) : ViewportOperationResult{true, {}};
  if (result.accepted && m_viewport) m_viewport->setSelectedObjects(m_document.selected);
  return result;
}

void OccViewWindow::removeObject(quint32 id)
{
  if (!m_viewport) return;
  m_viewport->removeObject(id);
  m_viewport->setSelectedObjects(m_document.selected);
}

ViewportOperationResult OccViewWindow::setObjectVisible(quint32 id, bool visible)
{
  const auto result = m_viewport ? m_viewport->setObjectVisible(id, visible) : ViewportOperationResult{true, {}};
  if (result.accepted && m_viewport) m_viewport->setSelectedObjects(m_document.selected);
  return result;
}

ViewportOperationResult OccViewWindow::setObjectDiagnostics(quint32 id, const DrawingDiagnostics& diagnostics)
{
  return m_viewport ? m_viewport->setObjectDiagnostics(id, diagnostics) : ViewportOperationResult{true, {}};
}

void OccViewWindow::setSelectedObjects(const QList<quint32>& ids)
{
  if (m_viewport) m_viewport->setSelectedObjects(ids);
}

ViewportOperationResult OccViewWindow::setDraftAxis(const std::optional<DrawingAxis>& axis)
{
  return m_viewport ? m_viewport->setDraftAxis(axis) : ViewportOperationResult{true, {}};
}

void OccViewWindow::setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet)
{
  m_pose.tcp = tcp;
  m_pose.collet = collet;
  if (m_viewport) m_viewport->setSpindleFrames(tcp, collet);
}

ViewportOperationResult OccViewWindow::applyTransforms(const DrawingPose& pose)
{
  if (!isReady()) return {false, QStringLiteral("Robot preview is unavailable.")};
  const auto result = m_viewport->applyTransforms(pose);
  if (result.accepted) {
    m_pose = pose;
    return result;
  }
  QString error = QStringLiteral("Robot pose update: ") + result.error;
  const auto restored = m_viewport->applyTransforms(m_pose);
  if (!restored.accepted) {
    error += QStringLiteral("; Robot pose restoration: ") + restored.error;
    m_presentationFailed = true;
    emit readyChanged();
  }
  return {false, error};
}

void OccViewWindow::setEndEffectors(
    const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes, RobotTool active)
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
  return !m_presentationFailed && m_viewport && m_viewport->isRobotReady();
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
  m_presentationFailed = false;
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
    m_presentationFailed = true;
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
  if (m_viewport || m_presentationFailed || !isExposed()) return;
  try {
    auto viewport = std::make_unique<OccViewport>(reinterpret_cast<Aspect_Handle>(winId()), m_document);
    if (!viewport->isValid()) {
      m_presentationFailed = true;
      emit errorOccurred(QStringLiteral("Cannot create the OCCT viewport."));
      return;
    }
    m_viewport = std::move(viewport);
    const auto axis = m_viewport->setDraftAxis(m_document.axis);
    if (!axis.accepted) emit errorOccurred(axis.error);
    m_viewport->setSpindleFrames(m_pose.tcp, m_pose.collet);
    m_viewport->setSelectedObjects(m_document.selected);
    m_viewport->setExposed(true);
  } catch (const Standard_Failure& failure) {
    releaseSurface();
    m_presentationFailed = true;
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
  if (!isViewportReady() || !m_visuals || !m_shapes || isReady() || m_presentationFailed) return;
  try {
    const auto result = m_viewport->setRobot(*m_visuals, *m_shapes, m_pose);
    m_presentationFailed = !result.accepted;
    if (m_presentationFailed) emit errorOccurred(result.error);
    else attachEndEffectors();
  } catch (const Standard_Failure& failure) {
    m_presentationFailed = true;
    emit errorOccurred(QStringLiteral("Robot installation: %1").arg(QString::fromUtf8(failure.what())));
  }
  emit readyChanged();
}

QPoint OccViewWindow::nativePosition(const QPointF& position) const
{
  return {static_cast<int>(std::lround(position.x() * devicePixelRatio())),
          static_cast<int>(std::lround(position.y() * devicePixelRatio()))};
}

} // namespace RoboCrap3D
