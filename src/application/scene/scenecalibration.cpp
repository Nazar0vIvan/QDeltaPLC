#include "scenecalibration.h"

#include "assetpaths.h"
#include "geometry/spherefit.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace {

struct SphereFileResult
{
  std::optional<SceneSphere> sphere;
  QString error;
};

std::optional<V3d> roughPoint(const QJsonValue& entry)
{
  const QJsonObject point = entry.toObject();
  const QJsonValue x = point.value(QStringLiteral("X"));
  const QJsonValue y = point.value(QStringLiteral("Y"));
  const QJsonValue z = point.value(QStringLiteral("Z"));
  if (!x.isDouble() || !y.isDouble() || !z.isDouble()) return std::nullopt;
  const V3d result{x.toDouble(), y.toDouble(), z.toDouble()};
  return result.allFinite() ? std::optional<V3d>{result} : std::nullopt;
}

std::optional<double> positiveNumber(const QJsonObject& root, const QString& key)
{
  const QJsonValue value = root.value(key);
  if (!value.isDouble() || !std::isfinite(value.toDouble()) || value.toDouble() <= 0.0) return std::nullopt;
  return value.toDouble();
}

// The table object: finite positive lengthX, widthY, sphereFromMinusX, sphereFromMinusY and
// sphereAboveTop (mm), with the sphere center over the table.
std::optional<CalibrationTable> calibrationTable(const QJsonObject& root)
{
  const QJsonObject table = root.value(QStringLiteral("table")).toObject();
  const auto lengthX = positiveNumber(table, QStringLiteral("lengthX"));
  const auto widthY = positiveNumber(table, QStringLiteral("widthY"));
  const auto fromMinusX = positiveNumber(table, QStringLiteral("sphereFromMinusX"));
  const auto fromMinusY = positiveNumber(table, QStringLiteral("sphereFromMinusY"));
  const auto aboveTop = positiveNumber(table, QStringLiteral("sphereAboveTop"));
  if (!lengthX || !widthY || !fromMinusX || !fromMinusY || !aboveTop
      || *fromMinusX >= *lengthX || *fromMinusY >= *widthY) return std::nullopt;
  return CalibrationTable{*lengthX, *widthY, *fromMinusX, *fromMinusY, *aboveTop};
}

// Version 1: finite positive sphereDiameter, strutDiameter and strutRecess (mm, the recess below
// the sphere diameter), the table object and roughPoints entries with X, Y, Z (mm) of the ruby
// ball centers; other keys are ignored. ballRadius (mm) compensates the fitted sphere.
SphereFileResult readSphereFile(const QString& path, double ballRadius)
{
  const QString prefix = SceneCalibration::tr("Calibration sphere file %1: ").arg(QDir::toNativeSeparators(path));
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {{}, prefix + SceneCalibration::tr("cannot be read (%1).").arg(file.errorString())};
  QJsonParseError parse;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse);
  if (parse.error != QJsonParseError::NoError || !document.isObject())
    return {{}, prefix + SceneCalibration::tr("is not a JSON object (%1).").arg(parse.errorString())};
  const QJsonObject root = document.object();
  if (root.value(QStringLiteral("version")).toInt() != 1)
    return {{}, prefix + SceneCalibration::tr("version must be 1.")};
  const auto diameter = positiveNumber(root, QStringLiteral("sphereDiameter"));
  const auto strutDiameter = positiveNumber(root, QStringLiteral("strutDiameter"));
  const auto strutRecess = positiveNumber(root, QStringLiteral("strutRecess"));
  if (!diameter || !strutDiameter || !strutRecess)
    return {{}, prefix + SceneCalibration::tr("sphereDiameter, strutDiameter and strutRecess must be finite positive numbers.")};
  if (*strutRecess >= *diameter)
    return {{}, prefix + SceneCalibration::tr("strutRecess must be smaller than sphereDiameter.")};
  const auto table = calibrationTable(root);
  if (!table)
    return {{}, prefix + SceneCalibration::tr("table needs finite positive lengthX, widthY, sphereFromMinusX, "
                                              "sphereFromMinusY and sphereAboveTop, with the sphere over the table.")};
  QList<V3d> points;
  bool finite = true;
  for (const QJsonValue& entry : root.value(QStringLiteral("roughPoints")).toArray()) {
    const auto point = roughPoint(entry);
    if (!point) { finite = false; break; }
    points.append(*point);
  }
  if (!finite || points.size() < 4)
    return {{}, prefix + SceneCalibration::tr("roughPoints needs at least 4 points with finite X, Y, Z.")};
  const auto fit = fitSphere(points);
  if (!fit)
    return {{}, prefix + SceneCalibration::tr("roughPoints do not determine a sphere (coplanar or degenerate).")};
  const double compensatedRadius = fit->radius - ballRadius;
  if (!(compensatedRadius > 0.0))
    return {{}, prefix + SceneCalibration::tr("the ball-center radius %1 mm does not exceed the ruby ball radius %2 mm.")
                    .arg(fit->radius, 0, 'f', 4).arg(ballRadius, 0, 'f', 4)};
  return {SceneSphere{fit->center, *diameter / 2.0, fit->radius, compensatedRadius, fit->rms, points,
                      *strutDiameter / 2.0, *strutRecess, *table}, {}};
}

bool calibrationObject(const SceneObject* object)
{
  return object->kind() == SceneObject::Sphere || object->kind() == SceneObject::CalibrationPath;
}

// Worker entry: the plan and motion from numerical copies only.
ProbeTouchMotionResult calibrationMotion(const ProbeTouchPlanParameters& parameters, const ChamferRobotSetup& robot)
{
  const auto plan = createProbeTouchPlan(parameters);
  if (!plan.plan) return {{}, plan.error};
  return ProbeTouchMotion::create(*plan.plan, robot);
}

} // namespace

SceneCalibration::SceneCalibration(SceneModel* scene, SceneEndEffectors* effectors, ScenePlayback* playback)
  : m_scene(scene), m_effectors(effectors), m_playback(playback)
{
  connect(scene, &SceneModel::objectAdded, this, &SceneCalibration::onObjectAdded);
  connect(effectors, &SceneEndEffectors::measuringTcpChanged, this, &SceneCalibration::invalidatePath);
  connect(playback, &ScenePlayback::robotModelChanged, this, &SceneCalibration::invalidatePath);
  connect(playback, &ScenePlayback::robotAvailabilityChanged, this, &SceneCalibration::availabilityChanged);
  connect(playback, &ScenePlayback::stateChanged, this, &SceneCalibration::availabilityChanged);
  connect(playback, &ScenePlayback::failed, this, &SceneCalibration::onPlaybackFailed);
  m_calculationTimer.setInterval(30);
  connect(&m_calculationTimer, &QTimer::timeout, this, &SceneCalibration::finishCalculation);
}

void SceneCalibration::setActive(bool active)
{
  if (m_active == active) return;
  m_active = active;
  if (active) activate();
  else deactivate();
}

void SceneCalibration::activate()
{
  auto file = readSphereFile(QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("resources/json/calibration_sphere.json")),
                             m_effectors->rubyBallDiameter() / 2.0);
  setError(file.error);
  if (file.sphere) {
    // Converted from resources/cad/CALIBRATION_STAND.stp at build time.
    file.sphere->standModel = QDir(RoboCrap3D::ViewportAssets::applicationAssets().cadDirectory)
                                  .filePath(QStringLiteral("CALIBRATION_STAND.brep"));
    replaceSphere(*file.sphere);
  }
  for (auto* object : m_scene->objectList()) hideOther(object);
  for (quint32 id : {m_sphereId, m_pathId})
    if (auto* object = m_scene->findObject(id)) m_scene->setObjectVisible(object, true);
}

void SceneCalibration::deactivate()
{
  if (ownsPlayback()) m_playback->stop();
  for (quint32 id : {m_sphereId, m_pathId})
    if (auto* object = m_scene->findObject(id)) m_scene->setObjectVisible(object, false);
  for (quint32 id : std::exchange(m_hiddenIds, {}))
    if (auto* object = m_scene->findObject(id)) m_scene->setObjectVisible(object, true);
}

const SceneSphere* SceneCalibration::sphere() const
{
  const auto* object = m_scene->findObject(m_sphereId);
  return object ? object->sphere() : nullptr;
}

const SceneTouchPath* SceneCalibration::path() const
{
  const auto* object = m_scene->findObject(m_pathId);
  return object ? object->touchPath() : nullptr;
}

bool SceneCalibration::ownsPlayback() const
{
  return m_pathId != 0 && m_playback->active() && m_playback->ownerId() == m_pathId;
}

void SceneCalibration::onPlaybackFailed(quint32 ownerId, const QString& error)
{
  if (ownerId != 0 && ownerId == m_pathId) setError(error);
}

bool SceneCalibration::canPlay() const
{
  if (calculating() || m_playback->playing() || !m_playback->robotAvailable()) return false;
  if (m_playback->active()) return ownsPlayback();
  return path() != nullptr;
}

void SceneCalibration::dryRun()
{
  if (!canPlay()) { setError(tr("Generate the calibration path before Dry Run.")); return; }
  if (m_playback->active()) { m_playback->resume(); return; }
  setError({});
  m_playback->start(path()->motion, m_pathId);
}

void SceneCalibration::replaceSphere(const SceneSphere& sphere)
{
  const auto* existing = this->sphere();
  if (existing && *existing == sphere) return;
  auto* inserted = m_scene->addCalibrationObject(sphere, tr("Calibration sphere"));
  if (!inserted) {
    setError(tr("Cannot insert the calibration sphere: scene IDs or drawing installation failed."));
    return;
  }
  if (existing) m_scene->removeObjects({m_sphereId});
  m_sphereId = inserted->objectId();
  invalidatePath();
}

void SceneCalibration::invalidatePath()
{
  ++m_revision;
  if (m_scene->findObject(m_pathId)) m_scene->removeObjects({m_pathId});
  m_pathId = 0;
  emit availabilityChanged();
}

QString SceneCalibration::unavailableReason() const
{
  if (calculating()) return tr("Calculating trajectory…");
  if (m_playback->active()) return tr("Stop Dry Run before generating a trajectory.");
  if (!m_playback->robotModel() || !m_playback->robotAvailable()) return tr("Robot preview is not ready.");
  if (!m_effectors->measuringTcpFrame()) return tr("Apply the measuring TCP first.");
  if (!sphere()) return tr("The calibration sphere is missing: check the sphere file.");
  return {};
}

void SceneCalibration::generate()
{
  const QString reason = unavailableReason();
  if (!reason.isEmpty()) { setError(reason); return; }
  const SceneSphere& value = *sphere();
  const ProbeTouchPlanParameters parameters{value.center, value.radius, m_effectors->rubyBallDiameter() / 2.0,
                                            value.strutRadius, value.strutRecess};
  ChamferRobotSetup robot{*m_playback->robotModel(), *m_effectors->measuringTcpFrame(), {}};
  m_calculationRevision = m_revision;
  setError({});
  // The worker owns numerical copies only; its result returns through the future.
  m_calculation = std::async(std::launch::async, &calibrationMotion, parameters, std::move(robot));
  m_calculationTimer.start();
  emit availabilityChanged();
}

void SceneCalibration::finishCalculation()
{
  if (m_calculation.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
  m_calculationTimer.stop();
  auto result = m_calculation.get();
  if (result.motion)
    qInfo().noquote() << QStringLiteral("Probe calibration preparation: %1 touches, %2 points, %3 s movement, "
                                        "minimum transfer clearance %4 mm, minimum strut clearance %5 mm")
                             .arg(ProbeTouchCount).arg(result.motion->points().size())
                             .arg(result.motion->duration(), 0, 'g', 12)
                             .arg(result.motion->minimumTransferClearance(), 0, 'f', 4)
                             .arg(result.motion->minimumStrutClearance(), 0, 'f', 4);
  else
    qInfo().noquote() << "Probe calibration preparation failed:" << result.error;
  if (m_revision != m_calculationRevision) setError(tr("Sphere, probe TCP or robot changed. Generate again."));
  else if (!result.motion) setError(result.error);
  else if (auto* inserted = m_scene->addCalibrationObject(
               SceneTouchPath{std::make_shared<const ProbeTouchMotion>(std::move(*result.motion))},
               tr("Calibration path"))) {
    if (!m_active) m_scene->setObjectVisible(inserted, false);
    if (m_scene->findObject(m_pathId)) m_scene->removeObjects({m_pathId});
    m_pathId = inserted->objectId();
  } else setError(tr("Cannot insert the calibration path: scene IDs or drawing installation failed."));
  emit availabilityChanged();
}

void SceneCalibration::hideOther(SceneObject* object)
{
  if (calibrationObject(object) || !object->visible()) return;
  m_hiddenIds.append(object->objectId());
  m_scene->setObjectVisible(object, false);
}

void SceneCalibration::onObjectAdded(SceneObject* object)
{
  if (m_active) hideOther(object);
}

void SceneCalibration::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  emit errorChanged();
}
