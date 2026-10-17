#include "scenemachiningpreparation.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStringList>
#include <QThread>
#include <chrono>
#include <utility>

namespace {

const QString SharedDataDirectory = QStringLiteral("C:/RoboCrapJobs");
const QString SharedDataPath = QStringLiteral("C:/RoboCrapJobs/shared_data.txt");

// The worker owns its copy of the saved motion handle.
ChamferRsiStreamResult sampleSavedMotion(std::shared_ptr<const ChamferMotion> motion)
{
  return sampleChamferRsiStream(*motion);
}

QString number(double value)
{
  return QString::number(value, 'f', 6);
}

QString frameRow(const V3d& position, const V3d& angles)
{
  return QStringList{number(position[0]), number(position[1]), number(position[2]),
                     number(angles[0]), number(angles[1]), number(angles[2])}
      .join(QLatin1Char(';')) + QStringLiteral("\r\n");
}

// Rows read by RSI_Hole.src: Ps, LeadStart, LeadEnd (X;Y;Z;A;B;C), $VEL.CP m/s, $ACC.CP m/s^2.
// Ps carries the lead-in start attitude; KRL derives its return attitude from LeadEnd.
QByteArray sharedDataPayload(const ChamferRsiStream& stream, const ChamferTimingParameters& timing)
{
  const V3d startAngles = stream.initialCoordinates.tail<3>();
  return (frameRow(stream.stagingPoint, startAngles)
          + frameRow(stream.initialCoordinates.head<3>(), startAngles)
          + frameRow(stream.finalCoordinates.head<3>(), stream.finalCoordinates.tail<3>())
          + number(timing.transferFeed / 1000.0) + QStringLiteral("\r\n")
          + number(timing.transferAcceleration / 1000.0) + QStringLiteral("\r\n")).toLatin1();
}

QString writeSharedData(const QByteArray& contents)
{
  if (!QDir().mkpath(SharedDataDirectory))
    return QStringLiteral("Cannot create %1.").arg(SharedDataDirectory);
  QSaveFile file(SharedDataPath);
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) {
    const QString error = file.errorString();
    file.cancelWriting();
    return QStringLiteral("Cannot write shared_data.txt: %1").arg(error);
  }
  if (!file.commit())
    return QStringLiteral("Cannot replace shared_data.txt: %1").arg(file.errorString());
  return {};
}

// First of three consecutive X/Y/Z or A/B/C components.
QString offsetRange(const ChamferRsiStream& stream, int first)
{
  static const char* const names[6] = {"X", "Y", "Z", "A", "B", "C"};
  QStringList ranges;
  for (int axis = first; axis < first + 3; ++axis)
    ranges.append(QStringLiteral("%1 [%2, %3]").arg(QLatin1String(names[axis]))
                      .arg(stream.minimumOffset[axis], 0, 'f', 3)
                      .arg(stream.maximumOffset[axis], 0, 'f', 3));
  return ranges.join(QStringLiteral(" · "));
}

} // namespace

SceneMachiningPreparation::SceneMachiningPreparation(SceneModel* scene, SceneMachining* machining,
                                                     QObject* parent)
  : QObject(parent), m_scene(scene), m_machining(machining)
{
  Q_ASSERT(scene && machining);
  Q_ASSERT(scene->thread() == thread() && machining->thread() == thread());
  // SceneMachining emits this after selection, removal and compatibility updates.
  connect(machining, &SceneMachining::availabilityChanged, this,
          &SceneMachiningPreparation::onAvailabilityChanged);
  m_timer.setInterval(30);
  connect(&m_timer, &QTimer::timeout, this, &SceneMachiningPreparation::finishPreparation);
  // No stream exists yet, so a file left by an earlier session is stale.
  clearPrepared();
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

bool SceneMachiningPreparation::isCurrent(const PreparedRsiStream& prepared) const
{
  const auto* object = m_scene->findObject(prepared.pathId);
  const auto* path = object ? qobject_cast<const SceneMachiningPath*>(object->geometry()) : nullptr;
  return path && path->compatible() && path->data().motion == prepared.motion;
}

QString SceneMachiningPreparation::unavailableReason() const
{
  if (m_shuttingDown) return tr("Application is shutting down.");
  if (m_preparing) return tr("Sampling RSI…");
  const auto* path = selectedPath();
  if (!path) return tr("Select a generated machining path.");
  return path->incompatibility();
}

QString SceneMachiningPreparation::summary() const
{
  if (!m_prepared) return {};
  const auto& stream = m_prepared->stream;
  // One multi-argument substitution: a '%' in the user-editable path name stays literal.
  return tr("RSI: %1 · %2 cycles · %3 s\nOffset mm: %4\nOffset deg: %5\nExported %6")
      .arg(m_prepared->pathName, QString::number(stream.increments.size()),
           QString::number(stream.duration, 'f', 3), offsetRange(stream, 0),
           offsetRange(stream, 3), SharedDataPath);
}

void SceneMachiningPreparation::onAvailabilityChanged()
{
  if (m_prepared && !isCurrent(*m_prepared)) clearPrepared();
  emit availabilityChanged();
}

void SceneMachiningPreparation::prepare()
{
  Q_ASSERT(QThread::currentThread() == thread());
  const QString reason = unavailableReason();
  if (!reason.isEmpty()) { setError(reason); return; }
  const auto* object = m_scene->findObject(m_machining->inputId());
  m_pending = {object->objectId(), object->name(), selectedPath()->data().motion, {}};
  // A new request replaces the previous stream and file, even if it fails.
  clearPrepared();
  setError({});
  m_preparing = true;
  m_future = std::async(std::launch::async, &sampleSavedMotion, m_pending.motion);
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
  if (result.stream)
    qInfo().noquote() << QStringLiteral("RSI Sampling: %1 cycles, %2 s movement")
                             .arg(result.stream->increments.size()).arg(result.stream->duration);
  else
    qInfo().noquote() << "RSI Sampling failed:" << result.error;
  if (!isCurrent(m_pending)) {
    setError(tr("The sampled path changed. Run RSI Sampling again."));
  } else if (!result.stream) {
    setError(result.error);
  } else {
    const QString writeError = writeSharedData(
        sharedDataPayload(*result.stream, m_pending.motion->robot().timing));
    if (writeError.isEmpty()) {
      m_pending.stream = std::move(*result.stream);
      m_prepared = std::move(m_pending);
      emit correctionsReady(m_prepared->stream.increments);
      emit preparedChanged();
    }
    setError(writeError);
  }
  m_pending = {};
  m_preparing = false;
  emit preparingChanged();
  emit availabilityChanged();
}

void SceneMachiningPreparation::clearPrepared()
{
  // Without a stream KRL must not find frames from another or older path.
  if (QFile::exists(SharedDataPath) && !QFile::remove(SharedDataPath))
    qInfo().noquote() << "[ERROR] RSI Sampling: cannot remove" << SharedDataPath;
  if (!m_prepared) return;
  m_prepared.reset();
  emit correctionsReady({});
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
  m_timer.stop();
  if (m_future.valid()) m_future.wait();
  m_future = {};
  m_pending = {};
  m_preparing = false;
  clearPrepared();
  emit preparingChanged();
  emit availabilityChanged();
}
