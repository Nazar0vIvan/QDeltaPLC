#include "scenemachiningpreparation.h"

#include "geometry/utils.h"
#include "kinematics/kr10kinematics.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>
#include <QThread>
#include <array>
#include <chrono>
#include <utility>

namespace {

const QString SharedDataDirectory = QStringLiteral("C:/RoboCrapJobs");
const QString SharedDataPath = QStringLiteral("C:/RoboCrapJobs/shared_data.txt");
const QString CalibrationDataPath = QStringLiteral("C:/RoboCrapJobs/calibration_data.txt");

// RSI_Probe_Check (resources/files/rsi/ProbeCheck/RSI_Probe_Check_CONTRACT.md): constant WORLD X increments.
constexpr double RsiCycleSeconds = 0.004; // #IPO_FAST
constexpr double TestLinStep = 0.004; // mm per cycle: 1 mm/s.
constexpr int TestLinCycles = 25000; // 100 mm.
constexpr int TestLinSettleCycles = 50; // Zero corrections before Result = 1, as for Sampling.

// The worker owns its copy of the saved motion handle.
ChamferRsiStreamResult sampleSavedMotion(std::shared_ptr<const ChamferMotion> motion)
{
  return sampleChamferRsiStream(*motion);
}

ProbeCalibrationRsiResult sampleSavedCalibration(std::shared_ptr<const ProbeTouchMotion> motion)
{
  return sampleProbeCalibrationRsi(*motion);
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

// Atomically replaces one job file in the folder that KRL mounts as /RoboCrapJobs.
QString replaceJobFile(const QString& path, const QByteArray& contents)
{
  const QString name = QFileInfo(path).fileName();
  if (!QDir().mkpath(SharedDataDirectory))
    return QStringLiteral("Cannot create %1.").arg(SharedDataDirectory);
  QSaveFile file(path);
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) {
    const QString error = file.errorString();
    file.cancelWriting();
    return QStringLiteral("Cannot write %1: %2").arg(name, error);
  }
  if (!file.commit())
    return QStringLiteral("Cannot replace %1: %2").arg(name, file.errorString());
  return {};
}

// One KRL POS row: six-decimal XYZABC and the step's S/T.
QString poseRow(const V6d& coordinates, const ProbeRsiStep& step)
{
  return frameRow(coordinates) + QStringLiteral(";%1;%2\r\n").arg(step.status).arg(step.turn);
}

// Version 1 (RSI_Probe_Calibration): ToolFrame, transfer $VEL.CP in m/s and $ACC.CP in m/s^2, the
// step count, then per step "kind;touch" and the target row (PTP, LIN), the auxiliary and target
// rows (CIRC) or no row (Search, HOME).
QString writeCalibrationData(const ProbeCalibrationRsi& job, const ChamferTimingParameters& timing)
{
  QString contents = QStringLiteral("1\r\n") + frameRow(job.toolCoordinates) + QStringLiteral("\r\n")
      + number(timing.transferFeed / 1000.0) + QStringLiteral("\r\n")
      + number(timing.transferAcceleration / 1000.0) + QStringLiteral("\r\n")
      + QString::number(job.steps.size()) + QStringLiteral("\r\n");
  for (const ProbeRsiStep& step : job.steps) {
    contents += QStringLiteral("%1;%2\r\n").arg(static_cast<int>(step.kind)).arg(step.touch);
    if (step.kind == ProbeRsiStepKind::Circ) contents += poseRow(step.auxiliary, step);
    if (step.kind != ProbeRsiStepKind::Search && step.kind != ProbeRsiStepKind::Home)
      contents += poseRow(step.target, step);
  }
  return replaceJobFile(CalibrationDataPath, contents.toLatin1());
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
  return replaceJobFile(SharedDataPath, contents);
}

} // namespace

SceneMachiningPreparation::SceneMachiningPreparation(SceneModel* scene, SceneMachining* machining,
                                                     SceneCalibration* calibration)
  : m_scene(scene), m_machining(machining), m_calibration(calibration)
{
  Q_ASSERT(scene && machining && calibration);
  Q_ASSERT(scene->thread() == thread() && machining->thread() == thread() && calibration->thread() == thread());
  // SceneMachining emits this after selection, removal and compatibility updates.
  connect(machining, &SceneMachining::availabilityChanged, this,
          &SceneMachiningPreparation::onAvailabilityChanged);
  // SceneCalibration emits this when its path is inserted, replaced or removed.
  connect(calibration, &SceneCalibration::availabilityChanged, this,
          &SceneMachiningPreparation::onCalibrationAvailabilityChanged);
  m_timer.setInterval(30);
  // One worker runs at a time; each completion slot ignores the other's future.
  connect(&m_timer, &QTimer::timeout, this, &SceneMachiningPreparation::finishPreparation);
  connect(&m_timer, &QTimer::timeout, this, &SceneMachiningPreparation::finishCalibration);
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

bool SceneMachiningPreparation::isCurrentCalibration(const std::shared_ptr<const ProbeTouchMotion>& motion) const
{
  const auto* path = m_calibration->path();
  return motion && path && path->motion == motion;
}

QString SceneMachiningPreparation::calibrationUnavailableReason() const
{
  if (m_shuttingDown) return tr("Application is shutting down.");
  if (m_preparing) return tr("RSI Sampling…");
  if (m_calibration->calculating()) return tr("Calculating trajectory…");
  if (!m_calibration->path()) return tr("Generate the calibration path first.");
  return {};
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
  if (m_preparedCalibration)
    return tr("RSI calibration: %1 touches · %2 steps")
        .arg(m_preparedCalibration->touchCount).arg(m_preparedCalibration->stepCount);
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

void SceneMachiningPreparation::onCalibrationAvailabilityChanged()
{
  if (m_preparedCalibration && !isCurrentCalibration(m_preparedCalibration->motion)) clearPrepared();
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

void SceneMachiningPreparation::prepareCalibration()
{
  Q_ASSERT(QThread::currentThread() == thread());
  const QString reason = calibrationUnavailableReason();
  if (!reason.isEmpty()) { setError(reason); return; }
  m_pendingCalibration = m_calibration->path()->motion;
  // A new request replaces the previous stream and job file, even if it fails.
  clearPrepared();
  setError({});
  m_preparing = true;
  m_calibrationFuture = std::async(std::launch::async, &sampleSavedCalibration, m_pendingCalibration);
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
  emit correctionsReady({increments}, QStringLiteral("test_lin_")
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
      emit correctionsReady({m_prepared->stream.increments}, m_prepared->stream.diagnosticId);
      emit preparedChanged();
    }
    setError(writeError);
  }
  m_pending = {};
  m_preparing = false;
  emit preparingChanged();
  emit availabilityChanged();
}

void SceneMachiningPreparation::finishCalibration()
{
  if (!m_calibrationFuture.valid()
      || m_calibrationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
  m_timer.stop();
  auto result = m_calibrationFuture.get();
  if (result.job)
    qInfo().noquote() << QStringLiteral("RSI calibration sampling: %1 touches, %2 steps")
                             .arg(result.job->touchCount).arg(result.job->steps.size());
  else
    qInfo().noquote() << "RSI calibration sampling failed:" << result.error;
  if (!isCurrentCalibration(m_pendingCalibration)) {
    setError(tr("The calibration path changed. Run RSI Sampling again."));
  } else if (!result.job) {
    setError(result.error);
  } else {
    const QString writeError = writeCalibrationData(*result.job, m_pendingCalibration->robot().timing);
    if (writeError.isEmpty()) {
      m_preparedCalibration = PreparedCalibration{m_pendingCalibration, result.job->touchCount,
                                                  result.job->steps.size()};
      emit correctionsReady(result.job->streams, result.job->diagnosticId);
      emit preparedChanged();
    }
    setError(writeError);
  }
  m_pendingCalibration.reset();
  m_preparing = false;
  emit preparingChanged();
  emit availabilityChanged();
}

void SceneMachiningPreparation::clearPrepared()
{
  // Without a stream KRL must not find frames from another or older path.
  for (const QString& path : {SharedDataPath, CalibrationDataPath})
    if (QFile::exists(path) && !QFile::remove(path))
      qInfo().noquote() << "[ERROR] RSI Sampling: cannot remove" << path;
  const bool wasPrepared = m_prepared.has_value() || m_testLin || m_preparedCalibration.has_value();
  m_prepared.reset();
  m_testLin = false;
  m_preparedCalibration.reset();
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
  if (m_calibrationFuture.valid()) m_calibrationFuture.wait();
  m_future = {};
  m_calibrationFuture = {};
  m_pending = {};
  m_pendingCalibration.reset();
  m_preparing = false;
  clearPrepared();
  emit preparingChanged();
  emit availabilityChanged();
}
