#include "occcontroller.h"
#include "scene/scenemachining.h"
#include "geometry/utils.h"

#include "occt/cadloadworker.h"
#include "occt/occviewwindow.h"
#include "robot/robotpreviewstate.h"
#include "scene/scenemodel.h"
#include "scene/sceneobject.h"
#include "viewportassets.h"

#include <QJSEngine>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QTimer>

#include <Standard_Failure.hxx>

#include <cmath>
#include <functional>
#include <optional>

namespace RoboCrap3D {

namespace {

QVariantList toList(const V6d& values)
{
  QVariantList result;
  result.reserve(6);
  for (double value : values) result.append(value);
  return result;
}

std::optional<V6d> readValues(const QVariantList& values)
{
  if (values.size() != 6) return std::nullopt;
  V6d result{};
  for (int i = 0; i < 6; ++i) {
    const int type = values[i].metaType().id();
    if (type != QMetaType::Double && type != QMetaType::Float
        && type != QMetaType::Int && type != QMetaType::UInt
        && type != QMetaType::LongLong && type != QMetaType::ULongLong) return std::nullopt;
    bool ok = false;
    result[i] = values[i].toDouble(&ok);
    if (!ok || !std::isfinite(result[i])) return std::nullopt;
  }
  return result;
}

std::optional<QStringList> robotCadSources(const RobotVisualModel& visuals, const QString& directory)
{
  QStringList sources;
  for (const LinkModel& link : visuals.links) {
    // Robot manifests contain basenames, not arbitrary paths.
    if (link.fileName.isEmpty() || QFileInfo(link.fileName).fileName() != link.fileName
        || link.fileName.contains('/') || link.fileName.contains('\\')) return std::nullopt;
    sources.append(QDir(directory).filePath(link.fileName));
  }
  return sources;
}

} // namespace

OccController::OccController(QObject* parent) : QObject(parent)
{
  QObject::connect(this, &OccController::poseChanged, this, &OccController::tcpPoseChanged);
  createWindow();
}

OccController::~OccController()
{
  if (m_machining) m_machining->setRobotAvailable(false);
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

QVariantList OccController::jointAngles() const
{
  return m_state ? toList(m_state->pose().joints()) : QVariantList{};
}

QVariantList OccController::flangePose() const
{
  return m_state ? toList(m_state->pose().flange()) : QVariantList{};
}

QList<double> OccController::tcpPose() const
{
  if (!m_state || !m_endEffectors) return {};
  QList<double> flange;
  for (double value : m_state->pose().flange()) flange.append(value);
  return m_endEffectors->spindlePoseFromFlange(flange);
}

void OccController::setApplicationScene(SceneModel* scene)
{
  if (m_applicationScene == scene) return;
  QObject::disconnect(m_sceneCollectionConnection);
  QObject::disconnect(m_sceneVisibilityConnection);
  QObject::disconnect(m_sceneRemovalConnection);
  QObject::disconnect(m_sceneDestroyedConnection);
  m_applicationScene = scene;
  if (scene) {
    m_sceneCollectionConnection = QObject::connect(
        scene, &SceneModel::objectAdded, this, &OccController::synchronizeSceneObject);
    m_sceneRemovalConnection = QObject::connect(scene, &SceneModel::objectRemoved, this, &OccController::removeSceneObject);
    m_sceneVisibilityConnection = QObject::connect(
        scene, &SceneModel::objectVisibilityChanged, this, &OccController::synchronizeSceneObject);
    m_sceneDestroyedConnection = QObject::connect(
        scene, &QObject::destroyed, this, &OccController::onSceneDestroyed);
  }
  if (m_window) m_window->setApplicationScene(scene);
}

void OccController::setMachining(SceneMachining* machining)
{
  if (m_machining == machining) return;
  if (m_machining) {
    disconnect(m_machining, nullptr, this, nullptr);
    m_machining->setRobotAvailable(false);
  }
  m_machining = machining;
  if (m_machining) {
    connect(m_machining, &SceneMachining::inputChanged, this, &OccController::synchronizeMachiningPreview);
    connect(m_machining, &SceneMachining::playbackPoseRequested, this,
            &OccController::applyPlaybackPose, Qt::DirectConnection);
    connect(m_machining, &SceneMachining::settingsChanged, this, &OccController::synchronizeMachiningPreview);
    connect(m_machining, &SceneMachining::availabilityChanged, this, &OccController::synchronizeMachiningPreview);
  }
  connect(this, &OccController::readyChanged, this,
          &OccController::synchronizeMachiningReadiness, Qt::UniqueConnection);
  if (m_machining && m_state) m_machining->setRobotModel(m_state->model().kinematics);
  synchronizeMachiningReadiness();
  synchronizeMachiningPreview();
}

void OccController::setShowMachiningPaths(bool visible)
{
  if (m_showMachiningPaths == visible) return;
  m_showMachiningPaths = visible;
  if (m_window) m_window->setMachiningPathsVisible(visible);
  emit showMachiningPathsChanged();
}

void OccController::synchronizeMachiningPreview()
{
  if (!m_window) return;
  m_window->setMachiningPreview(m_machining ? m_machining->previewParameters() : std::nullopt);
}

void OccController::synchronizeMachiningReadiness()
{
  if (m_machining) m_machining->setRobotAvailable(isReady() && !m_loading);
}

void OccController::onSceneDestroyed()
{
  if (m_window) m_window->setApplicationScene(nullptr);
}

bool OccController::isMeasuringCadLoading() const
{
  return m_toolCad[SceneEndEffectors::Measuring].worker != nullptr;
}

bool OccController::isSpindleCadLoading() const
{
  return m_toolCad[SceneEndEffectors::Spindle].worker != nullptr;
}

QString OccController::measuringCadError() const
{
  return m_toolCad[SceneEndEffectors::Measuring].error;
}

QString OccController::spindleCadError() const
{
  return m_toolCad[SceneEndEffectors::Spindle].error;
}

void OccController::setEndEffectors(SceneEndEffectors* effectors)
{
  if (effectors && m_endEffectors == effectors) return;
  if (m_endEffectors) QObject::disconnect(m_endEffectors, nullptr, this, nullptr);
  m_endEffectors = effectors;
  if (effectors) {
    QObject::connect(effectors, &SceneEndEffectors::activeToolChanged,
                     this, &OccController::synchronizeEndEffectors);
    QObject::connect(effectors, &SceneEndEffectors::spindleTcpChanged,
                     this, &OccController::tcpPoseChanged);
    QObject::connect(effectors, &SceneEndEffectors::spindleTcpChanged,
                     this, &OccController::synchronizeSpindleTcp);
    QObject::connect(effectors, &SceneEndEffectors::measuringCadSourceChanged, this,
                     std::bind(&OccController::reloadEndEffector, this, SceneEndEffectors::Measuring));
    QObject::connect(effectors, &SceneEndEffectors::spindleCadSourceChanged, this,
                     std::bind(&OccController::reloadEndEffector, this, SceneEndEffectors::Spindle));
    QObject::connect(effectors, &QObject::destroyed, this,
                     std::bind(&OccController::setEndEffectors, this, nullptr));
  }
  for (ToolCadState& tool : m_toolCad) {
    tool.loadedSource.clear();
    tool.shapes.reset();
  }
  reloadEndEffector(SceneEndEffectors::Measuring);
  reloadEndEffector(SceneEndEffectors::Spindle);
  emit tcpPoseChanged();
  synchronizeEndEffectors();
}

void OccController::synchronizeEndEffectors()
{
  if (!m_window) return;
  m_window->setEndEffectors({m_toolCad[0].shapes, m_toolCad[1].shapes},
      m_endEffectors ? m_endEffectors->activeTool() : SceneEndEffectors::Measuring);
  synchronizeSpindleTcp();
}

void OccController::setShowSpindleTcp(bool visible)
{
  if (m_showSpindleTcp == visible) return;
  m_showSpindleTcp = visible;
  synchronizeSpindleTcp();
  emit showSpindleTcpChanged();
}

void OccController::synchronizeSpindleTcp()
{
  if (!m_window) return;
  std::optional<M4d> frame;
  if (m_showSpindleTcp && m_endEffectors
      && m_endEffectors->activeTool() == SceneEndEffectors::Spindle) {
    const auto& tcp = m_endEffectors->spindleTcp();
    if (tcp.size() == 6) {
      const M4d transform = makeTransform(euler2rot(tcp[3], tcp[4], tcp[5]),
                                          V3d{tcp[0], tcp[1], tcp[2]});
      if (transform.allFinite()) frame = transform;
    }
  }
  m_window->setSpindleTcpFrame(frame);
}

void OccController::reloadEndEffector(SceneEndEffectors::Tool tool)
{
  if (m_shuttingDown || (tool != SceneEndEffectors::Measuring && tool != SceneEndEffectors::Spindle)) return;
  ToolCadState& state = m_toolCad[tool];
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

void OccController::startEndEffectorLoad(SceneEndEffectors::Tool tool)
{
  if (m_shuttingDown) return;
  ToolCadState& state = m_toolCad[tool];
  if (!m_endEffectors) {
    emit endEffectorLoadChanged();
    return;
  }
  const QUrl source = tool == SceneEndEffectors::Measuring
      ? m_endEffectors->measuringCadSource() : m_endEffectors->spindleCadSource();
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
  state.worker = std::make_unique<CadLoadWorker>(sources, ViewportAssets::applicationAssets().cacheDirectory);
  QObject::connect(state.worker.get(), &QThread::finished, this,
                   std::bind(&OccController::finishEndEffectorLoad, this, tool, state.generation, source),
                   Qt::QueuedConnection);
  state.worker->start();
  emit endEffectorLoadChanged();
}

void OccController::finishEndEffectorLoad(SceneEndEffectors::Tool tool, quint64 generation, const QUrl& source)
{
  if (m_shuttingDown) return;
  ToolCadState& state = m_toolCad[tool];
  state.worker->wait();
  auto result = std::make_shared<CadLoadResult>(state.worker->takeResult());
  state.worker.reset();
  if (generation != state.generation) {
    startEndEffectorLoad(tool);
    return;
  }
  if (result->error.isEmpty() && result->shapes.size() == 1) {
    state.loadedSource = source;
    state.shapes = std::move(result);
    synchronizeEndEffectors();
    emit endEffectorLoaded(tool);
  } else {
    state.error = result->error.isEmpty() ? QStringLiteral("CAD import produced no tool shape.") : result->error;
    const QString name = tool == SceneEndEffectors::Measuring ? QStringLiteral("MEE") : QStringLiteral("SEE");
    emit message(QStringLiteral("%1 CAD (%2): %3").arg(name, source.toLocalFile(), state.error), true);
  }
  emit endEffectorLoadChanged();
}

void OccController::removeSceneObject(quint32 objectId)
{
  if (m_window) m_window->removeSceneObject(objectId);
}

void OccController::synchronizeSceneObject(SceneObject* object)
{
  if (m_window) m_window->synchronizeSceneObject(object);
  if (m_machining && object && object->objectId() == m_machining->inputId()) synchronizeMachiningPreview();
}

void OccController::setSelectedObjects(const QList<quint32>& ids)
{
  if (m_window) m_window->setSelectedObjects(ids);
}

void OccController::setDiagnosticOverlays(bool showPoints, bool showNormals)
{
  if (m_window) m_window->setDiagnosticOverlays(showPoints, showNormals);
}

void OccController::createWindow()
{
  if (m_window || m_shuttingDown) return;
  if (!m_occtInitialized) {
    QString error;
    m_occtInitialized = ViewportAssets::applicationAssets().initializeOcct(error);
    if (!m_occtInitialized) {
      setError(error);
      return;
    }
  }
  m_window = new OccViewWindow();
  QJSEngine::setObjectOwnership(m_window, QJSEngine::CppOwnership);
  QObject::connect(m_window, &OccViewWindow::readyChanged, this, &OccController::readyChanged);
  QObject::connect(m_window, &OccViewWindow::viewportReadyChanged, this, &OccController::viewportReadyChanged);
  QObject::connect(m_window, &OccViewWindow::applicationSelectionRequested,
                   this, &OccController::applicationSelectionRequested);
  QObject::connect(m_window, &OccViewWindow::errorOccurred, this, &OccController::setError);
  QObject::connect(m_window, &QObject::destroyed, this, &OccController::onViewWindowDestroyed);
  QObject::connect(m_window, &OccViewWindow::deleteSelectionRequested, this, &OccController::deleteSelectionRequested);
  m_window->setApplicationScene(m_applicationScene);
  m_window->setMachiningPathsVisible(m_showMachiningPaths);
  synchronizeMachiningPreview();
  if (m_state && m_shapes) m_window->setScene(m_state, m_shapes);
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

void OccController::loadRobot()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_loading || m_shuttingDown) return;
  createWindow();
  if (!m_window) return;
  if (m_state && m_shapes) {
    setError({});
    if (!isReady()) m_window->setScene(m_state, m_shapes);
    return;
  }

  const ViewportAssets assets = ViewportAssets::applicationAssets();
  QString error;

  std::shared_ptr<RobotPreviewState> pending;
  try {
    const auto model = Kr10Model::fromJson(assets.modelFile);
    if (!model) {
      setError(QStringLiteral("Cannot read the robot definition: %1").arg(assets.modelFile));
      return;
    }
    pending = std::make_shared<RobotPreviewState>(*model);
    if (!pending->initialize(error)) {
      setError(error);
      return;
    }
  } catch (const Standard_Failure& failure) {
    setError(QStringLiteral("Invalid robot definition: %1").arg(QString::fromUtf8(failure.what())));
    return;
  }

  const auto sources = robotCadSources(pending->model().visuals, assets.cadDirectory);
  if (!sources) {
    setError(QStringLiteral("The robot definition contains an invalid CAD filename."));
    return;
  }
  setError({});
  m_loading = true;
  emit loadingChanged();
  emit readyChanged();
  const quint64 generation = ++m_generation;
  m_worker = std::make_unique<CadLoadWorker>(*sources, assets.cacheDirectory);
  QObject::connect(m_worker.get(), &QThread::finished, this,
                   std::bind(&OccController::finishRobotLoad, this, generation, pending),
                   Qt::QueuedConnection);
  m_worker->start();
}

void OccController::onViewWindowDestroyed()
{
  if (m_shuttingDown) return;
  emit readyChanged();
  emit viewportReadyChanged();
  // A host may destroy its child QWindow before QML's detach handler runs.
  // QPointer prevents double deletion; the persistent robot state is retained.
  QTimer::singleShot(0, this, &OccController::createWindow);
}

void OccController::finishRobotLoad(quint64 generation, std::shared_ptr<RobotPreviewState> pending)
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
  m_state = pending;
  if (IgnorePreviewJointPositionLimits)
    emit message(QStringLiteral("Offline diagnostic: A1-A6 joint-position limits are disabled in the robot preview."), false);
  if (m_machining) m_machining->setRobotModel(m_state->model().kinematics);
  m_shapes = result;
  if (m_window)
    m_window->setScene(m_state, m_shapes);
  else
    createWindow();
  emit poseChanged();
  emit readyChanged();
}

bool OccController::applyPose(const RobotPose& pose)
{
  if (!isReady() || !m_state) return false;
  try {
    if (!m_window->applyPose(pose)) {
      m_window->setScene(m_state, m_shapes);
      return false;
    }
    m_state->commit(pose);
    emit poseChanged();
    return true;
  } catch (const Standard_Failure&) {
    // Restore only the robot from the last committed state after a partial update.
    m_window->setScene(m_state, m_shapes);
    return false;
  }
}

QVariantList OccController::solve(const QVariantList& values, bool inverse)
{
  if (m_machining && m_machining->playing()) {
    setError(QStringLiteral("Stop Dry Run before changing the robot pose."));
    return {};
  }
  Q_ASSERT(QThread::currentThread() == thread());
  if (!isReady() || !m_state) {
    setError(QStringLiteral("The robot preview is not ready."));
    return {};
  }
  const auto input = readValues(values);
  if (!input) {
    setError(QStringLiteral("Enter exactly six finite numeric values."));
    return {};
  }
  try {
    QString error;
    const auto pose = inverse ? m_state->inverse(*input, error) : m_state->forward(*input, error);
    if (!pose) {
      setError(error);
      return {};
    }
    if (!applyPose(*pose)) {
      setError(QStringLiteral("Cannot update the robot presentation."));
      return {};
    }
    setError({});
    return toList(inverse ? pose->joints() : pose->flange());
  } catch (const Standard_Failure& failure) {
    setError(QStringLiteral("Kinematics failed: %1").arg(QString::fromUtf8(failure.what())));
    return {};
  }
}

QVariantList OccController::solveFK(const QVariantList& joints)
{
  return solve(joints, false);
}

void OccController::applyPlaybackPose(QList<double> joints)
{
  if (!m_machining || !m_machining->playing()) return;
  if (!isReady() || m_loading || !m_state || joints.size() != 6) {
    m_machining->failPlayback(QStringLiteral("Robot preview is not ready for playback."));
    return;
  }
  V6d values = V6d::Zero();
  for (int i = 0; i < 6; ++i) values[i] = joints[i];
  QString error;
  const auto pose = m_state->forward(values, error);
  if (!pose || !applyPose(*pose)) {
    m_machining->failPlayback(error.isEmpty() ? QStringLiteral("Cannot display the trajectory pose.") : error);
  }
}

QVariantList OccController::solveIK(const QVariantList& flange)
{
  return solve(flange, true);
}

QVariantList OccController::solveTcpIK(const QVariantList& tcp)
{
  const auto values = readValues(tcp);
  if (!values) {
    setError(QStringLiteral("Enter exactly six finite numeric TCP values."));
    return {};
  }
  if (!m_endEffectors || m_endEffectors->spindleTcp().isEmpty()) {
    setError(QStringLiteral("Configure the spindle TCP relative to the flange first."));
    return {};
  }
  QList<double> target;
  for (double value : *values) target.append(value);
  const QList<double> flange = m_endEffectors->flangePoseFromSpindle(target);
  if (flange.isEmpty()) {
    setError(QStringLiteral("The TCP target and calibration produced an invalid flange pose."));
    return {};
  }
  QVariantList flangeValues;
  for (double value : flange) flangeValues.append(value);
  return solveIK(flangeValues);
}

} // namespace RoboCrap3D
