#include "viewport/occcontroller.h"

#include "occt/cadloadworker.h"
#include "occt/occviewwindow.h"

#include <QJSEngine>
#include <QDir>
#include <QThread>
#include <QTimer>

#include <functional>
#include <utility>

#include <Font_FontMgr.hxx>
#include <Standard_Failure.hxx>

namespace RoboCrap3D {

OccController::OccController(QString cacheDirectory, QString occtResources, QObject* parent)
  : QObject(parent), m_cacheDirectory(std::move(cacheDirectory)), m_occtResources(std::move(occtResources))
{
  createWindow();
}

OccController::~OccController()
{
  m_shuttingDown = true;
  ++m_generation;
  for (ToolCadState& tool : m_toolCad)
    if (tool.worker) tool.worker->requestInterruption();
  if (m_worker) {
    m_worker->requestInterruption();
    m_worker->wait();
  }
  for (ToolCadState& tool : m_toolCad)
    if (tool.worker) tool.worker->wait();
  if (m_window) {
    detachViewWindow(m_window);
    delete m_window.data();
  }
}

QWindow* OccController::viewWindow() const
{
  return m_window.data();
}

bool OccController::isReady() const
{
  return !m_loading && m_window && m_window->isReady();
}

bool OccController::isViewportReady() const
{
  return m_window && m_window->isViewportReady();
}

ViewportOperationResult OccController::insertObject(const DrawingObject& object)
{
  if (!object.id || object.parts.isEmpty()) return {false, QStringLiteral("Empty object drawing.")};
  const auto result = m_window ? m_window->insertObject(object) : ViewportOperationResult{true, {}};
  if (result.accepted) m_document.objects.insert(object.id, object);
  if (!result.error.isEmpty()) emit message(result.error, true);
  return result;
}

void OccController::removeObject(quint32 id)
{
  m_document.objects.remove(id);
  if (m_window) m_window->removeObject(id);
}

void OccController::setObjectVisible(quint32 id, bool visible)
{
  auto it = m_document.objects.find(id);
  if (it == m_document.objects.end() || it->visible == visible) return;
  const auto result = m_window ? m_window->setObjectVisible(id, visible) : ViewportOperationResult{true, {}};
  if (result.accepted) it->visible = visible;
  if (!result.error.isEmpty()) emit message(result.error, true);
}

void OccController::setObjectDiagnostics(quint32 id, const DrawingDiagnostics& diagnostics)
{
  auto it = m_document.objects.find(id);
  if (it == m_document.objects.end()) return;
  const auto result = m_window ? m_window->setObjectDiagnostics(id, diagnostics) : ViewportOperationResult{true, {}};
  if (result.accepted) it->diagnostics = diagnostics;
  if (!result.error.isEmpty()) emit message(result.error, true);
}

void OccController::setDraftAxis(const std::optional<DrawingAxis>& axis)
{
  const auto& previous = m_document.axis;
  if ((!axis && !previous) || (axis && previous && axis->center == previous->center
      && axis->direction == previous->direction && axis->length == previous->length)) return;
  const auto result = m_window ? m_window->setDraftAxis(axis) : ViewportOperationResult{true, {}};
  if (result.accepted) m_document.axis = axis;
  if (!result.error.isEmpty()) emit message(result.error, true);
}

bool OccController::isMeasuringCadLoading() const
{
  return m_toolCad[0].worker != nullptr;
}

bool OccController::isSpindleCadLoading() const
{
  return m_toolCad[1].worker != nullptr;
}

QString OccController::measuringCadError() const
{
  return m_toolCad[0].error;
}

QString OccController::spindleCadError() const
{
  return m_toolCad[1].error;
}

void OccController::synchronizeEndEffectors()
{
  if (!m_window) return;
  m_window->setEndEffectors({m_toolCad[0].shapes, m_toolCad[1].shapes},
      m_activeTool);
}

void OccController::setShowSpindleTcp(bool visible)
{
  if (m_showSpindleTcp == visible) return;
  m_showSpindleTcp = visible;
  emit showSpindleTcpChanged();
}

void OccController::setShowSpindleCollet(bool visible)
{
  if (m_showSpindleCollet == visible) return;
  m_showSpindleCollet = visible;
  emit showSpindleColletChanged();
}

void OccController::loadEndEffector(RobotTool tool, const QUrl& source)
{
  if (m_shuttingDown || (tool != RobotTool::Measuring && tool != RobotTool::Spindle)) return;
  ToolCadState& state = m_toolCad[static_cast<std::size_t>(tool)];
  state.source = source;
  ++state.generation;
  state.error.clear();
  if (state.worker) {
    // Finish the current parse before replacing its worker; discard its stale result.
    state.worker->requestInterruption();
    emit endEffectorLoadChanged();
    return;
  }
  startEndEffectorLoad(tool);
}

void OccController::setActiveTool(RobotTool tool)
{
  if (m_activeTool == tool) return;
  m_activeTool = tool;
  synchronizeEndEffectors();
}

void OccController::setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet)
{
  m_pose.tcp = tcp;
  m_pose.collet = collet;
  if (m_window) m_window->setSpindleFrames(tcp, collet);
}

void OccController::startEndEffectorLoad(RobotTool tool)
{
  if (m_shuttingDown) return;
  ToolCadState& state = m_toolCad[static_cast<std::size_t>(tool)];
  const QUrl source = state.source;
  if (source.isEmpty()) {
    emit endEffectorLoadChanged();
    return;
  }
  if (!m_occtInitialized) {
    state.error = QStringLiteral("OCCT resources must be initialized before loading tool CAD.");
    emit endEffectorLoadChanged();
    return;
  }
  const QStringList sources{source.toLocalFile()};
  state.worker = std::make_unique<CadLoadWorker>(sources, m_cacheDirectory);
  QObject::connect(state.worker.get(), &QThread::finished, this,
                   std::bind(&OccController::finishEndEffectorLoad, this, tool, state.generation, source),
                   Qt::QueuedConnection);
  state.worker->start();
  emit endEffectorLoadChanged();
}

void OccController::finishEndEffectorLoad(RobotTool tool, quint64 generation, const QUrl& source)
{
  if (m_shuttingDown) return;
  ToolCadState& state = m_toolCad[static_cast<std::size_t>(tool)];
  state.worker->wait();
  auto result = std::make_shared<CadLoadResult>(state.worker->takeResult());
  state.worker.reset();
  if (generation != state.generation) {
    startEndEffectorLoad(tool);
    return;
  }
  if (result->error.isEmpty() && result->shapes.size() == 1) {
    state.shapes = std::move(result);
    synchronizeEndEffectors();
    emit endEffectorLoaded(tool);
  } else {
    state.error = result->error.isEmpty() ? QStringLiteral("CAD import produced no tool shape.") : result->error;
    const QString name = tool == RobotTool::Measuring ? QStringLiteral("MEE") : QStringLiteral("SEE");
    emit message(QStringLiteral("%1 CAD (%2): %3").arg(name, source.toLocalFile(), state.error), true);
  }
  emit endEffectorLoadChanged();
}

void OccController::setSelectedObjects(const QList<quint32>& ids)
{
  m_document.selected = ids;
  if (m_window) m_window->setSelectedObjects(ids);
}

QString OccController::initializeOcct() const
{
  const QDir resources(m_occtResources);
  if (!resources.exists(QStringLiteral("Shaders")) || !resources.exists(QStringLiteral("XSTEPResource"))) {
    return QStringLiteral("OCCT runtime resources are missing: %1").arg(m_occtResources);
  }

  const std::pair<const char*, const char*> paths[] = {
    {"CSF_ShadersDirectory", "Shaders"}, {"CSF_MDTVTexturesDirectory", "Textures"},
    {"CSF_SHMessage", "SHMessage"}, {"CSF_XSMessage", "XSMessage"},
    {"CSF_STEPDefaults", "XSTEPResource"}, {"CSF_PluginDefaults", "StdResource"},
    {"CSF_StandardDefaults", "StdResource"}, {"CSF_XCAFDefaults", "StdResource"}
  };
  for (const auto& [variable, directory] : paths)
    qputenv(variable, resources.filePath(QLatin1String(directory)).toUtf8());
  try {
    Font_FontMgr::GetInstance()->InitFontDataBase();
    return {};
  } catch (const Standard_Failure& failure) {
    return QStringLiteral("Cannot initialize OCCT: %1").arg(QString::fromUtf8(failure.what()));
  }
}

void OccController::createWindow()
{
  if (m_window || m_shuttingDown) return;
  if (!m_occtInitialized) {
    const QString error = initializeOcct();
    m_occtInitialized = error.isEmpty();
    if (!m_occtInitialized) {
      setError(error);
      return;
    }
  }
  m_window = new OccViewWindow(m_document);
  QJSEngine::setObjectOwnership(m_window, QJSEngine::CppOwnership);
  QObject::connect(m_window, &OccViewWindow::readyChanged, this, &OccController::readyChanged);
  QObject::connect(m_window, &OccViewWindow::viewportReadyChanged, this, &OccController::viewportReadyChanged);
  QObject::connect(m_window, &OccViewWindow::applicationSelectionRequested,
                   this, &OccController::applicationSelectionRequested);
  QObject::connect(m_window, &OccViewWindow::errorOccurred, this, &OccController::setError);
  QObject::connect(m_window, &QObject::destroyed, this, &OccController::onViewWindowDestroyed);
  QObject::connect(m_window, &OccViewWindow::deleteSelectionRequested, this, &OccController::deleteSelectionRequested);
  if (m_visuals && m_shapes) m_window->setScene(*m_visuals, m_shapes, m_pose);
  synchronizeEndEffectors();
  emit viewWindowChanged();
}

void OccController::detachViewWindow(QWindow* window)
{
  if (!m_window || window != m_window.data()) return;
  m_window->hide();
  m_window->releaseSurface();
  m_window->setParent(nullptr);
}

void OccController::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  emit errorStringChanged();
  if (!error.isEmpty()) emit message(error, true);
}

ViewportOperationResult OccController::loadRobot(const RobotVisualModel& visuals,
                                                 const DrawingPose& pose)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_shuttingDown) return {false, QStringLiteral("The viewport is shutting down.")};
  if (m_visuals) return {m_error.isEmpty(), m_error};
  createWindow();
  if (!m_window) return {false, m_error};
  m_visuals = visuals;
  m_pose = pose;
  QStringList sources;
  for (const auto& link : visuals.links) sources.append(link.fileName);
  setError({});
  m_loading = true;
  emit loadingChanged();
  emit readyChanged();
  const quint64 generation = ++m_generation;
  m_worker = std::make_unique<CadLoadWorker>(sources, m_cacheDirectory);
  QObject::connect(m_worker.get(), &QThread::finished, this,
                   std::bind(&OccController::finishRobotLoad, this, generation), Qt::QueuedConnection);
  m_worker->start();
  return {true, {}};
}

void OccController::onViewWindowDestroyed()
{
  if (m_shuttingDown) return;
  emit readyChanged();
  emit viewportReadyChanged();
  // A host may destroy its child QWindow before QML's detach handler runs.
  // QPointer prevents double deletion; renderer replay values are retained.
  QTimer::singleShot(0, this, &OccController::createWindow);
}

void OccController::finishRobotLoad(quint64 generation)
{
  if (m_shuttingDown || generation != m_generation) return;
  m_worker->wait();
  auto result = std::make_shared<CadLoadResult>(m_worker->takeResult());
  m_worker.reset();
  m_loading = false;
  emit loadingChanged();
  if (!result->error.isEmpty()) {
    setError(result->error);
    emit readyChanged();
    return;
  }
  m_shapes = std::move(result);
  if (m_window)
    m_window->setScene(*m_visuals, m_shapes, m_pose);
  else
    createWindow();
  emit readyChanged();
}

ViewportOperationResult OccController::applyRobotTransforms(const DrawingPose& pose)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!isReady()) return {false, QStringLiteral("The robot preview is not ready.")};
  const auto result = m_window->applyTransforms(pose);
  if (result.accepted) {
    m_pose = pose;
    setError({});
  } else if (m_error == result.error) {
    emit message(result.error, true);
  } else {
    setError(result.error);
  }
  return result;
}

} // namespace RoboCrap3D
