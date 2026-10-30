#include "scenemachiningpreparation.h"

#include "geometry/utils.h"
#include "kinematics/kr10kinematics.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStringList>
#include <QThread>
#include <array>
#include <chrono>
#include <utility>

namespace {

const QString SharedDataDirectory = QStringLiteral("C:/RoboCrapJobs");
const QString SharedDataPath = QStringLiteral("C:/RoboCrapJobs/shared_data.txt");

// RSI_Probe_Lin (resources/files/rsi/RSI_Probe_Lin_CONTRACT.md): constant WORLD X increments.
constexpr double RsiCycleSeconds = 0.004; // #IPO_FAST
constexpr double TestLinStep = 0.004; // mm per cycle: 1 mm/s.
constexpr int TestLinCycles = 25000; // 100 mm.
constexpr int TestLinSettleCycles = 50; // Zero corrections before Result = 1, as for Sampling.

// The worker owns its copy of the saved motion handle.
ChamferRsiStreamResult sampleSavedMotion(std::shared_ptr<const ChamferMotion> motion)
{
  return sampleChamferRsiStream(*motion);
}

QString number(double value)
{
  return QString::number(value, 'f', 6);
}

QString frameRow(const V6d& coordinates)
{
  QStringList fields;
  for (double value : coordinates) fields.append(number(value));
  return fields.join(QLatin1Char(';'));
}

// Version 2: PsIn/LeadStart/LeadEnd XYZABC+S/T, PsOut S/T, ToolFrame, CP rates.
// KRL constructs PsOut from PsIn XYZ and LeadEnd ABC; each point has its own S/T.
QString writeSharedData(const ChamferRsiStream& stream, const ChamferMotion& motion)
{
  const RoboCrap3D::Kr10Kinematics kinematics(motion.robot().model);
  const V3d staging = ((stream.stagingPoint * 1e6).array().round() / 1e6).matrix();
  constexpr std::array<std::pair<std::size_t, const char*>, 4> targets{{
      {1, "PsIn"}, {2, "LeadStart"}, {5, "LeadEnd"}, {6, "PsOut"}}};
  QByteArray contents("2\r\n");
  for (const auto& [boundary, name] : targets) {
    const auto& point = motion.points()[motion.boundaries()[boundary]];
    V6d coordinates = boundary <= 2 ? stream.initialCoordinates : stream.finalCoordinates;
    if (boundary == 1 || boundary == 6) coordinates.head<3>() = staging;
    const auto angles = coordinates.tail<3>();
    const M3d rotation = euler2rot(angles[0], angles[1], angles[2]);
    if (!coordinates.allFinite()
        || (coordinates.head<3>() - point.tcp.block<3, 1>(0, 3)).norm() > RoboCrap3D::Kr10PosEps
        || (rotation - point.tcp.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > RoboCrap3D::Kr10RotEps)
      return QStringLiteral("Exported %1 does not match its saved animation TCP pose.")
          .arg(QString::fromLatin1(name));
    const auto frames = kinematics.solveFK(point.joints);
    const auto configuration = kinematics.configuration(point.joints, frames);
    const QString statusTurn = QStringLiteral("%1;%2\r\n")
        .arg(configuration.status).arg(configuration.turn);
    contents += (boundary == 6 ? statusTurn
                              : frameRow(coordinates) + QLatin1Char(';') + statusTurn).toLatin1();
  }
  const auto& timing = motion.robot().timing;
  contents += (frameRow(stream.toolCoordinates) + QStringLiteral("\r\n")
               + number(timing.transferFeed / 1000.0) + QStringLiteral("\r\n")
               + number(timing.transferAcceleration / 1000.0) + QStringLiteral("\r\n")).toLatin1();
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

// Only machining paths have a geometry QObject.
SceneMachiningPath* SceneMachiningPreparation::pathById(quint32 objectId) const
{
  const auto* object = m_scene->findObject(objectId);
  return object ? qobject_cast<SceneMachiningPath*>(object->geometry()) : nullptr;
}

bool SceneMachiningPreparation::isCurrent(const PreparedRsiStream& prepared) const
{
  const auto* path = pathById(prepared.pathId);
  return path && path->compatible() && path->data().motion == prepared.motion;
}

QString SceneMachiningPreparation::unavailableReason() const
{
  if (m_shuttingDown) return tr("Application is shutting down.");
  if (m_preparing) return tr("RSI Sampling…");
  if (m_machining->calculating()) return tr("Calculating trajectory…");
  const auto* path = pathById(m_machining->inputId());
  if (!path) return tr("Select a generated machining path.");
  return path->incompatibility();
}

QString SceneMachiningPreparation::summary() const
{
  if (m_testLin)
    return tr("RSI test LIN: WORLD +X %1 mm at %2 mm/s · %3 cycles")
        .arg(TestLinCycles * TestLinStep).arg(TestLinStep / RsiCycleSeconds).arg(TestLinCycles + TestLinSettleCycles);
  if (!m_prepared) return {};
  const auto& stream = m_prepared->stream;
  // One line for the footer status bar. One multi-argument substitution keeps a
  // '%' in the user-editable path name literal.
  return tr("RSI: %1 · %2 cycles · %3 s · offset up to %4 mm, %5°")
      .arg(m_prepared->pathName, QString::number(stream.increments.size()),
           QString::number(stream.duration, 'f', 3),
           QString::number(stream.maximumTranslationOffset, 'f', 3),
           QString::number(stream.maximumRotationOffset, 'f', 3));
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
  const quint32 pathId = m_machining->inputId();
  m_pending = {pathId, m_scene->findObject(pathId)->name(), pathById(pathId)->data().motion, {}};
  // A new request replaces the previous stream and file, even if it fails.
  clearPrepared();
  setError({});
  m_preparing = true;
  m_future = std::async(std::launch::async, &sampleSavedMotion, m_pending.motion);
  m_timer.start();
  emit preparingChanged();
  emit availabilityChanged();
}

void SceneMachiningPreparation::prepareTestLin()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_shuttingDown || m_preparing) {
    setError(m_shuttingDown ? tr("Application is shutting down.") : tr("RSI Sampling…"));
    return;
  }
  // Like a new Sampling request: RSI_Chamfer_Hole must not find a file for another stream.
  clearPrepared();
  QVector<V6d> increments(TestLinCycles + TestLinSettleCycles, V6d::Zero());
  for (int cycle = 0; cycle < TestLinCycles; ++cycle) increments[cycle][0] = TestLinStep;
  setError({});
  m_testLin = true;
  emit correctionsReady(increments, QStringLiteral("test_lin_")
      + QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
  emit preparedChanged();
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
    const QString writeError = writeSharedData(*result.stream, *m_pending.motion);
    if (writeError.isEmpty()) {
      m_pending.stream = std::move(*result.stream);
      m_prepared = std::move(m_pending);
      emit correctionsReady(m_prepared->stream.increments, m_prepared->stream.diagnosticId);
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
  const bool wasPrepared = m_prepared.has_value() || m_testLin;
  m_prepared.reset();
  m_testLin = false;
  emit correctionsReady({}, {});
  if (wasPrepared) emit preparedChanged();
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
