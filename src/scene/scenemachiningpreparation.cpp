#include "scenemachiningpreparation.h"

#include <QThread>
#include <QUuid>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <chrono>
#include <utility>

SceneMachiningPreparation::SceneMachiningPreparation(SceneModel* scene, SceneMachining* machining,
                                                     QObject* parent)
  : QObject(parent), m_scene(scene), m_machining(machining)
{
  Q_ASSERT(scene && machining);
  Q_ASSERT(scene->thread() == thread() && machining->thread() == thread());
  connect(machining, &SceneMachining::inputChanged, this, &SceneMachiningPreparation::onInputChanged);
  connect(machining, &SceneMachining::availabilityChanged, this,
          &SceneMachiningPreparation::onAvailabilityChanged);
  connect(scene, &SceneModel::objectRemoved, this, &SceneMachiningPreparation::onObjectRemoved);
  m_timer.setInterval(30);
  connect(&m_timer, &QTimer::timeout, this, &SceneMachiningPreparation::finishPreparation);
  m_limitPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("rsi-correction-limits.json"));
  connect(&m_limitWatcher, &QFileSystemWatcher::fileChanged, this,
          &SceneMachiningPreparation::onLimitFileChanged);
  connect(&m_limitWatcher, &QFileSystemWatcher::directoryChanged, this,
          &SceneMachiningPreparation::onLimitFileChanged);
  refreshLimitConfiguration();
  observeSelectedPath();
}

SceneMachiningPreparation::~SceneMachiningPreparation()
{
  shutdown();
}

SceneMachiningPath* SceneMachiningPreparation::selectedPath() const
{
  const auto* object = m_scene->findObject(m_machining->inputId());
  if (!object || object->kind() != SceneObject::MachiningPath) return nullptr;
  return qobject_cast<SceneMachiningPath*>(object->geometry());
}

QString SceneMachiningPreparation::unavailableReason() const
{
  if (m_shuttingDown) return tr("Application is shutting down.");
  if (m_preparing) return tr("Preparing job…");
  if (m_machining->calculating()) return tr("Finish generating the trajectory first.");
  if (m_machining->playbackActive()) return tr("Stop Dry Run before preparing a job.");
  const auto* path = selectedPath();
  if (!path || !path->data().motion) return tr("Select a generated machining path.");
  return path->incompatibility();
}

bool SceneMachiningPreparation::sourceIsCurrent(const ChamferJobSource& source) const
{
  const auto* path = selectedPath();
  return path && path->compatible() && m_machining->inputId() == source.pathId
      && path->data().motion == source.motion
      && path->data().calibrationRevision == source.calibrationRevision;
}

void SceneMachiningPreparation::observeSelectedPath()
{
  QObject::disconnect(m_compatibilityConnection);
  m_path = selectedPath();
  if (m_path)
    m_compatibilityConnection = connect(m_path, &SceneMachiningPath::compatibilityChanged, this,
                                        &SceneMachiningPreparation::onAvailabilityChanged);
}

void SceneMachiningPreparation::onInputChanged()
{
  ++m_generation;
  observeSelectedPath();
  setPreparedJob({});
  setError({});
  emit availabilityChanged();
}

void SceneMachiningPreparation::onAvailabilityChanged()
{
  if (m_preparing && (!sourceIsCurrent(m_pendingSource) || m_machining->calculating()
                      || m_machining->playbackActive()))
    ++m_generation;
  if (m_job && !sourceIsCurrent(m_job->source())) setPreparedJob({});
  emit availabilityChanged();
}

void SceneMachiningPreparation::onObjectRemoved(quint32 id)
{
  if (m_preparing && m_pendingSource.pathId == id) ++m_generation;
  if (m_job && m_job->source().pathId == id) setPreparedJob({});
  emit availabilityChanged();
}

void SceneMachiningPreparation::onLimitFileChanged(const QString&)
{
  if (!m_shuttingDown) refreshLimitConfiguration();
}

void SceneMachiningPreparation::refreshLimitConfiguration()
{
  Q_ASSERT(QThread::currentThread() == thread());
  const QFileInfo file(m_limitPath);
  const QString directory = file.absolutePath();
  // Watch the directory too: creation and atomic replacement can remove a file watch.
  // Preparation also rereads directly before dispatch and before accepting a result.
  if (!m_limitWatcher.directories().contains(directory)) m_limitWatcher.addPath(directory);
  if (file.exists() && !m_limitWatcher.files().contains(m_limitPath)) m_limitWatcher.addPath(m_limitPath);
  auto configuration = readRsiCorrectionLimitConfiguration(m_limitPath);
  if (configuration.revision == m_limitConfiguration.revision
      && configuration.error == m_limitConfiguration.error
      && configuration.supplied == m_limitConfiguration.supplied) return;
  m_limitConfiguration = std::move(configuration);
  ++m_generation;
  setPreparedJob({});
  setError({});
  emit availabilityChanged();
}

void SceneMachiningPreparation::prepare()
{
  Q_ASSERT(QThread::currentThread() == thread());
  const QString reason = unavailableReason();
  if (!reason.isEmpty()) { setError(reason); return; }
  refreshLimitConfiguration();
  const auto* path = selectedPath();
  m_pendingSource = {path->data().motion,
                     QUuid::createUuid().toString(QUuid::WithoutBraces),
                     m_machining->inputId(), path->sourceEdgeId(), path->data().calibrationRevision};
  m_pendingGeneration = ++m_generation;
  m_preparing = true;
  setPreparedJob({});
  setError({});
  m_future = std::async(std::launch::async, &PreparedChamferJob::create, m_pendingSource, m_limitConfiguration);
  m_timer.start();
  emit preparingChanged();
  emit availabilityChanged();
}

void SceneMachiningPreparation::finishPreparation()
{
  if (!m_future.valid()
      || m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
  m_timer.stop();
  auto result = m_future.get();
  refreshLimitConfiguration();
  const bool current = !m_shuttingDown && m_pendingGeneration == m_generation
      && sourceIsCurrent(m_pendingSource) && !m_machining->calculating()
      && !m_machining->playbackActive();
  m_pendingSource = {};
  if (!current) {
    setError(tr("Scene, robot or RSI limits changed. Prepare again."));
  } else if (!result.job) {
    setError(result.error);
  } else {
    setError({});
    setPreparedJob(std::move(result.job));
  }
  m_preparing = false;
  emit preparingChanged();
  emit availabilityChanged();
}

void SceneMachiningPreparation::setPreparedJob(PreparedChamferJobPtr job)
{
  if (m_job == job) return;
  m_job = std::move(job);
  emit preparedJobReady(m_job);
  emit preparedChanged();
}

void SceneMachiningPreparation::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  emit errorChanged();
}

void SceneMachiningPreparation::shutdown()
{
  if (m_shuttingDown) return;
  m_shuttingDown = true;
  ++m_generation;
  m_timer.stop();
  m_limitWatcher.blockSignals(true);
  setPreparedJob({});
  if (m_future.valid()) {
    m_future.wait();
    m_future = {};
  }
  m_pendingSource = {};
  m_preparing = false;
  emit preparingChanged();
  emit availabilityChanged();
}
