#include "chamferprofile.h"
#include "3d/robot/model/kr10model.h"
#include "scene/sceneendeffectors.h"
#include "pathgeneration/chamfer/chamferjob.h"

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSysInfo>
#include <limits>

namespace {

QList<double> poseValues(const QJsonValue& value)
{
  if (!value.isArray() || value.toArray().size() != 6) return {};
  QList<double> result;
  for (const auto& coordinate : value.toArray()) {
    if (!coordinate.isDouble()) return {};
    result.append(coordinate.toDouble());
  }
  return result;
}

// Geometry, robot setup and diagnostic scope determine which production calculation runs.
QJsonObject runPreparation(const ChamferPath& path, const ChamferRobotSetup& robot, bool previewOnly)
{
  auto generated = ChamferMotion::create(path, robot);
  QJsonObject result{{"preview", generated.diagnostics}, {"previewError", generated.error}};
  if (!generated.motion) return result;
  result.insert("previewDurationSeconds", generated.motion->duration());
  result.insert("centralDurationSeconds", generated.motion->centralMotion()->duration());
  if (previewOnly) return result;
  ChamferJobSource source;
  source.motion = std::make_shared<const ChamferMotion>(std::move(*generated.motion));
  source.jobId = QStringLiteral("offline-profile");
  source.pathId = 1;
  auto prepared = PreparedChamferJob::create(std::move(source));
  result.insert("rsi", prepared.diagnostics);
  result.insert("rsiError", prepared.error);
  if (prepared.job) {
    result.insert("rsiCorrections", prepared.job->correctionCount());
    result.insert("encodingReady", prepared.job->encodingReady());
    result.insert("encodingStatus", prepared.job->encodingUnavailableReason());
  }
  return result;
}

} // namespace

int profileChamferPreparation(const QString& inputPath, const QString& reportPath)
{
  QFile file(inputPath);
  if (!file.open(QIODevice::ReadOnly) || file.size() > 65536) {
    qWarning() << "Cannot read profiling input (maximum 64 KiB).";
    return 2;
  }
  const auto document = QJsonDocument::fromJson(file.readAll());
  if (!document.isObject()) { qWarning() << "Profiling input must be a JSON object."; return 2; }
  const auto input = document.object();
  if (input.contains("previewOnly") && !input.value("previewOnly").isBool()) {
    qWarning() << "previewOnly must be boolean.";
    return 2;
  }
  const bool previewOnly = input.value("previewOnly").toBool(false);
  const QDir directory = QFileInfo(inputPath).absoluteDir();
  const auto plane = BoundedPlane::fromJsonFile(directory.filePath(input.value("plane").toString()));
  const auto cylinder = BoundedCylinder::fromJsonFile(directory.filePath(input.value("cylinder").toString()));
  const auto model = RoboCrap3D::RobotDefinition::fromJson(directory.filePath(input.value("robot").toString()));
  if (!plane || !cylinder || !model) { qWarning() << "Cannot load profiling geometry or robot."; return 2; }
  const auto intersection = intersectSurfaces(*plane, *cylinder);
  if (!intersection.edge) { qWarning() << "Profiling intersection failed."; return 2; }
  const auto settings = input.value("settings").toObject();
  for (const auto* key : {"leadInClearance", "leadOutClearance", "leadInSpan", "leadOutSpan"})
    if (!settings.value(QLatin1String(key)).isDouble()) {
      qWarning() << "Missing numeric spatial lead setting:" << key;
      return 2;
    }
  const double missing = std::numeric_limits<double>::quiet_NaN();
  ChamferPathParameters parameters;
  parameters.edge = intersection.edge->ellipse();
  const auto& origin = cylinder->origin();
  const auto& axis = cylinder->axis();
  parameters.cylinderOrigin = V3d{origin[0], origin[1], origin[2]};
  parameters.cylinderAxis = V3d{axis[0], axis[1], axis[2]};
  if (!settings.value("flipAxis").isBool()) { qWarning() << "Missing flipAxis."; return 2; }
  parameters.flipAxis = settings.value("flipAxis").toBool();
  parameters.chamferSize = settings.value("chamferSize").toDouble(missing);
  parameters.chamferAngleDegrees = settings.value("chamferAngle").toDouble(missing);
  parameters.stagingDistance = settings.value("stagingDistance").toDouble(missing);
  parameters.leadIn.clearance = settings.value("leadInClearance").toDouble(missing);
  parameters.leadOut.clearance = settings.value("leadOutClearance").toDouble(missing);
  parameters.leadIn.spanDegrees = settings.value("leadInSpan").toDouble(missing);
  parameters.leadOut.spanDegrees = settings.value("leadOutSpan").toDouble(missing);
  QElapsedTimer geometryTimer;
  geometryTimer.start();
  const auto path = ChamferPath::create(parameters);
  const double geometryMs = geometryTimer.nsecsElapsed() / 1e6;
  if (!path.path) { qWarning().noquote() << path.error; return 2; }
  SceneEndEffectors effectors;
  const auto tcpInCollet = poseValues(input.value("tcpInCollet"));
  const bool calibrated = input.contains("colletPose")
      ? effectors.applySpindlePoses(poseValues(input.value("colletPose")), tcpInCollet)
      : effectors.setSpindleTcpInCollet(tcpInCollet);
  if (!calibrated) { qWarning() << "Invalid profiling TCP calibration."; return 2; }
  ChamferRobotSetup robot;
  robot.model = model->kinematics;
  robot.flangeToTcp = *effectors.spindleTcpFrame();
  robot.timing.machiningFeed = settings.value("machiningFeed").toDouble(missing);
  robot.timing.accelerationDistanceFraction = settings.value("accelerationDistanceFraction").toDouble(missing);
  robot.timing.transferFeed = settings.value("transferFeed").toDouble(missing);
  robot.timing.transferAcceleration = settings.value("transferAcceleration").toDouble(missing);
  robot.timing.auxiliaryScale = settings.value("auxiliaryScale").toDouble(missing);
  const int repetitions = input.value("repetitions").toInt(1);
  if (repetitions < 1 || repetitions > 10) { qWarning() << "Repetitions must be 1 to 10."; return 2; }
  QJsonObject report{{"input", input}, {"qtVersion", qVersion()},
                     {"host", QSysInfo::machineHostName()}, {"architecture", QSysInfo::currentCpuArchitecture()}};
#ifdef QT_DEBUG
  report.insert("build", "Debug");
#else
  report.insert("build", "Release");
#endif
  report.insert("scope", previewOnly ? "preview" : "preview-and-rsi");
  report.insert("geometryConstructionMs", geometryMs);
  QJsonArray calibration;
  for (double coordinate : effectors.spindleTcp()) calibration.append(coordinate);
  report.insert("flangeToTcpXYZABC", calibration);
  report.insert("cartesianAccelerationBound", robot.timing.cartesianAcceleration);
  report.insert("jointAccelerationBound", robot.timing.jointAcceleration[0]);
  QJsonArray runs;
  for (int index = 0; index < repetitions; ++index) {
    qInfo() << "Offline chamfer profiling run" << index + 1 << "of" << repetitions;
    auto run = runPreparation(*path.path, robot, previewOnly);
    qInfo().noquote() << QJsonDocument(run).toJson(QJsonDocument::Compact);
    runs.append(run);
    report.insert("runs", runs);
    QSaveFile output(reportPath);
    const auto bytes = QJsonDocument(report).toJson(QJsonDocument::Indented);
    if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) {
      qWarning() << "Cannot write profiling report:" << output.errorString();
      return 2;
    }
    if (!run.value("previewError").toString().isEmpty()) return 1;
  }
  return 0;
}
