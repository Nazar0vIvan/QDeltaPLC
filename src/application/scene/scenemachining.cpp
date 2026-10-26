#include "scenemachining.h"

#include <utility>
#include <QDebug>
#include <algorithm>
#include <chrono>

SceneMachiningSettings SceneMachiningSettings::fromMotion(const ChamferMotion& motion)
{
  SceneMachiningSettings result;
  const auto& path = motion.parameters();
  const auto& timing = motion.robot().timing;
  result.chamferSize = path.chamferSize;
  result.chamferAngle = path.chamferAngleDegrees;
  result.stagingDistance = path.stagingDistance;
  result.flipAxis = path.flipAxis;
  result.leadInClearance = path.leadIn.clearance;
  result.leadOutClearance = path.leadOut.clearance;
  result.leadInSpan = path.leadIn.spanDegrees;
  result.leadOutSpan = path.leadOut.spanDegrees;
  result.machiningFeed = timing.machiningFeed;
  result.accelerationDistanceFraction = timing.accelerationDistanceFraction;
  result.transferFeed = timing.transferFeed;
  result.transferAcceleration = timing.transferAcceleration;
  result.acceleration = timing.cartesianAcceleration;
  result.auxiliaryScale = timing.auxiliaryScale;
  for (int i = 0; i < 6; ++i) {
    result.jointSpeed[i] = timing.jointSpeed[i];
    result.jointAcceleration[i] = timing.jointAcceleration[i];
  }
  return result;
}

ChamferPathParameters SceneMachiningSettings::applyTo(ChamferPathParameters parameters) const
{
  parameters.chamferSize = chamferSize;
  parameters.chamferAngleDegrees = chamferAngle;
  parameters.stagingDistance = stagingDistance;
  parameters.flipAxis = flipAxis;
  parameters.leadIn.clearance = leadInClearance;
  parameters.leadOut.clearance = leadOutClearance;
  parameters.leadIn.spanDegrees = leadInSpan;
  parameters.leadOut.spanDegrees = leadOutSpan;
  return parameters;
}

std::optional<ChamferTimingParameters> SceneMachiningSettings::timing() const
{
  if (jointSpeed.size() != 6 || jointAcceleration.size() != 6) return std::nullopt;
  ChamferTimingParameters result;
  result.machiningFeed = machiningFeed;
  result.accelerationDistanceFraction = accelerationDistanceFraction;
  result.transferFeed = transferFeed;
  result.transferAcceleration = transferAcceleration;
  result.cartesianAcceleration = acceleration;
  result.auxiliaryScale = auxiliaryScale;
  for (int i = 0; i < 6; ++i) {
    result.jointSpeed[i] = jointSpeed[i];
    result.jointAcceleration[i] = jointAcceleration[i];
  }
  return result;
}

SceneMachining::SceneMachining(SceneModel* scene, SceneEndEffectors* effectors, ScenePlayback* playback)
  : m_scene(scene), m_effectors(effectors), m_playback(playback)
{
  connect(effectors, &SceneEndEffectors::spindleTcpChanged, this, &SceneMachining::robotSetupChanged);
  connect(scene, &SceneModel::objectRemoved, this, &SceneMachining::onObjectRemoved);
  connect(scene, &SceneModel::objectAdded, this, &SceneMachining::onObjectAdded);
  connect(scene, &SceneModel::objectVisibilityChanged, this, &SceneMachining::synchronizeDraftAxis);
  connect(this, &SceneMachining::settingsChanged, this, &SceneMachining::synchronizeDraftAxis);
  connect(this, &SceneMachining::inputChanged, this, &SceneMachining::synchronizeDraftAxis);
  connect(this, &SceneMachining::availabilityChanged, this, &SceneMachining::synchronizeDraftAxis);
  m_calculationTimer.setInterval(30);
  connect(&m_calculationTimer, &QTimer::timeout, this, &SceneMachining::finishCalculation);
  connect(playback, &ScenePlayback::stateChanged, this, &SceneMachining::availabilityChanged);
  connect(playback, &ScenePlayback::failed, this, &SceneMachining::onPlaybackFailed);
  connect(playback, &ScenePlayback::robotModelChanged, this, &SceneMachining::applyRobotModel);
  connect(playback, &ScenePlayback::robotAvailabilityChanged, this, &SceneMachining::refreshCompatibility);
}

void SceneMachining::setSettings(const SceneMachiningSettings& settings)
{
  m_settings = settings;
  setError({});
  emit settingsChanged();
}

void SceneMachining::setInputId(quint32 id)
{
  if (id == m_inputId) return;
  m_inputId = id;
  const auto* object = m_scene->findObject(id);
  if (object && object->machiningPath()) {
    setSettings(SceneMachiningSettings::fromMotion(*object->machiningPath()));
  } else if (const auto source = sourceParameters()) {
    // Current workpiece convention: the opening points toward negative scene X.
    auto settings = m_settings;
    settings.flipAxis = source->cylinderAxis.x() > 0.0;
    setSettings(settings);
  }
  setError({});
  emit inputChanged();
  emit availabilityChanged();
}

std::optional<ChamferPathParameters> SceneMachining::sourceParameters() const
{
  const auto* object = m_scene->findObject(m_inputId);
  if (!object) return std::nullopt;
  if (const auto* motion = object->machiningPath()) return motion->parameters();
  const auto* edge = object->edge();
  if (!edge) return std::nullopt;
  const BoundedCylinder* cylinder = nullptr;
  bool hasPlane = false;
  for (quint32 id : edge->sources) {
    const auto* source = m_scene->findObject(id);
    if (!source) return std::nullopt;
    if (source->plane()) hasPlane = true;
    if (source->cylinder()) cylinder = source->cylinder();
  }
  if (!hasPlane || !cylinder) return std::nullopt;
  ChamferPathParameters parameters;
  parameters.edge = edge->edge.ellipse();
  const auto& origin = cylinder->origin();
  const auto& axis = cylinder->axis();
  parameters.cylinderOrigin = V3d{origin[0], origin[1], origin[2]};
  parameters.cylinderAxis = V3d{axis[0], axis[1], axis[2]};
  return parameters;
}

QString SceneMachining::unavailableReason() const
{
  if (m_calculating) return tr("Calculating trajectory…");
  if (m_playback->active()) return tr("Stop Dry Run before generating a trajectory.");
  if (!m_playback->robotModel() || !m_playback->robotAvailable()) return tr("Robot preview is not ready.");
  if (!m_effectors->spindleTcpFrame()) return tr("Apply the spindle TCP first.");
  if (!sourceParameters()) return tr("Select an intersection edge with its source plane and cylinder, or a generated machining path.");
  return {};
}

void SceneMachining::synchronizeDraftAxis()
{
  std::optional<RoboCrap3D::DrawingAxis> axis;
  const auto* object = m_scene->findObject(m_inputId);
  const auto source = object && object->visible() ? sourceParameters() : std::nullopt;
  if (source) {
    V3d direction = source->cylinderAxis;
    const double norm = direction.stableNorm();
    if (direction.allFinite() && norm > GeomConst::Eps) {
      direction /= norm;
      if (m_settings.flipAxis) direction = -direction;
      axis = RoboCrap3D::DrawingAxis{source->edge.center, direction,
                                    std::clamp(source->edge.minorRadius, 5.0, 40.0)};
    }
  }
  emit draftAxisChanged(axis);
}

void SceneMachining::generate()
{
  if (m_calculating) return;
  const QString reason = unavailableReason();
  if (!reason.isEmpty()) { setError(reason); return; }
  const auto source = sourceParameters();
  const auto timing = m_settings.timing();
  if (!source || !timing) { setError(tr("Six joint speed and acceleration limits are required.")); return; }
  const auto path = ChamferPath::create(m_settings.applyTo(*source));
  if (!path.path) { setError(path.error); return; }
  ChamferRobotSetup robot;
  robot.model = *m_playback->robotModel();
  robot.flangeToTcp = *m_effectors->spindleTcpFrame();
  robot.timing = *timing;
  m_calculationInputId = m_inputId;
  m_calculationRevision = m_revision;
  setError({});
  // The worker owns numerical copies only; its result returns through the future.
  m_calculation = std::async(std::launch::async, &ChamferMotion::create, *path.path, std::move(robot));
  setCalculating(true);
  m_calculationTimer.start();
}

void SceneMachining::setCalculating(bool calculating)
{
  if (m_calculating == calculating) return;
  m_calculating = calculating;
  emit calculatingChanged();
  emit availabilityChanged();
}

void SceneMachining::finishCalculation()
{
  if (!m_calculation.valid()) return;
  if (m_calculation.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
  m_calculationTimer.stop();
  auto result = m_calculation.get();
  if (result.motion)
    qInfo().noquote() << QStringLiteral("Chamfer preparation: %1 points, %2 s movement")
                             .arg(result.motion->points().size()).arg(result.motion->duration(), 0, 'g', 12);
  else
    qInfo().noquote() << "Chamfer preparation failed:" << result.error;
  const auto* previous = m_scene->findObject(m_calculationInputId);
  if (!previous || m_inputId != m_calculationInputId || m_revision != m_calculationRevision
      || !m_playback->robotAvailable() || !sourceParameters()) {
    setError(tr("Scene or robot configuration changed. Generate again."));
    setCalculating(false);
    return;
  }
  if (!result.motion) {
    setError(result.error);
    setCalculating(false);
    return;
  }
  const auto* saved = qobject_cast<const SceneMachiningPath*>(previous->geometry());
  const quint32 replaceId = saved ? previous->objectId() : 0;
  const quint32 sourceId = saved ? saved->sourceEdgeId() : previous->objectId();
  const QString name = saved ? previous->name() : QString{};
  const bool visible = previous->visible();
  SceneMachiningData data{std::make_shared<const ChamferMotion>(std::move(*result.motion)), sourceId, m_calculationRevision};
  auto* inserted = m_scene->addMachiningPath(std::move(data), name);
  if (!inserted) {
    setError(tr("Cannot insert trajectory: scene IDs or drawing installation failed."));
    setCalculating(false);
    return;
  }
  const quint32 id = inserted->objectId();
  if (replaceId) {
    m_scene->setObjectVisible(inserted, visible);
    m_scene->removeObjects({replaceId});
  }
  setInputId(id);
  setError({});
  emit generated(id);
  setCalculating(false);
}

void SceneMachining::applyRobotModel()
{
  auto settings = m_settings;
  settings.jointSpeed.clear();
  for (const auto& joint : m_playback->robotModel()->joints)
    settings.jointSpeed.append(joint.ratedPayloadSpeedDegreesPerSecond);
  setSettings(settings);
  robotSetupChanged();
}

void SceneMachining::robotSetupChanged()
{
  if (ownsPlayback()) m_playback->end();
  ++m_revision;
  refreshCompatibility();
}

void SceneMachining::refreshCompatibility()
{
  for (auto* object : m_scene->objectList()) updateCompatibility(object);
  emit availabilityChanged();
}

void SceneMachining::updateCompatibility(SceneObject* object)
{
  auto* path = qobject_cast<SceneMachiningPath*>(object->geometry());
  if (!path) return;
  QString reason;
  if (path->data().robotSetupRevision != m_revision)
    reason = tr("Robot model or TCP changed. Regenerate this trajectory.");
  else if (!m_playback->robotAvailable()) reason = tr("Robot preview is not ready.");
  path->setIncompatibility(reason);
}

void SceneMachining::onObjectAdded(SceneObject* object)
{
  updateCompatibility(object);
  emit availabilityChanged();
}

void SceneMachining::onObjectRemoved(quint32 id)
{
  if (m_inputId == id) setInputId(0);
  else emit availabilityChanged();
}

void SceneMachining::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  emit errorChanged();
}

const SceneMachiningPath* SceneMachining::machiningPath(quint32 id) const
{
  const auto* object = m_scene->findObject(id);
  return object ? qobject_cast<const SceneMachiningPath*>(object->geometry()) : nullptr;
}

bool SceneMachining::ownsPlayback() const
{
  return m_playback->active() && machiningPath(m_playback->ownerId());
}

void SceneMachining::onPlaybackFailed(quint32 ownerId, const QString& error)
{
  if (machiningPath(ownerId)) setError(error);
}

bool SceneMachining::canPlay() const
{
  if (m_calculating || m_playback->playing() || !m_playback->robotAvailable()) return false;
  if (m_playback->active()) return ownsPlayback();
  const auto* path = machiningPath(m_inputId);
  return path && path->compatible();
}

void SceneMachining::dryRun()
{
  if (!canPlay()) { setError(tr("Select a compatible machining path before Dry Run.")); return; }
  if (m_playback->active()) { m_playback->resume(); return; }
  setError({});
  m_playback->start(machiningPath(m_inputId)->data().motion, m_inputId);
}

void SceneMachining::stop()
{
  if (ownsPlayback()) m_playback->stop();
}
