#include "scenemachining.h"

#include "geometry/utils.h"

#include <utility>
#include <algorithm>
#include <chrono>
#include <QCoreApplication>
#include <QMetaMethod>

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
  result.leadInIntervals = path.leadIn.intervals;
  result.leadOutIntervals = path.leadOut.intervals;
  result.leadInFeed = timing.leadInFeed;
  result.machiningFeed = timing.machiningFeed;
  result.leadOutFeed = timing.leadOutFeed;
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
  parameters.leadIn = {leadInClearance, leadInSpan, leadInIntervals};
  parameters.leadOut = {leadOutClearance, leadOutSpan, leadOutIntervals};
  return parameters;
}

std::optional<ChamferTimingParameters> SceneMachiningSettings::timing() const
{
  if (jointSpeed.size() != 6 || jointAcceleration.size() != 6) return std::nullopt;
  ChamferTimingParameters result;
  result.leadInFeed = leadInFeed;
  result.machiningFeed = machiningFeed;
  result.leadOutFeed = leadOutFeed;
  result.cartesianAcceleration = acceleration;
  result.auxiliaryScale = auxiliaryScale;
  for (int i = 0; i < 6; ++i) {
    result.jointSpeed[i] = jointSpeed[i];
    result.jointAcceleration[i] = jointAcceleration[i];
  }
  return result;
}

SceneMachining::SceneMachining(SceneModel* scene, SceneEndEffectors* effectors, QObject* parent)
  : QObject(parent), m_scene(scene), m_effectors(effectors)
{
  connect(effectors, &SceneEndEffectors::spindleTcpChanged, this, &SceneMachining::calibrationChanged);
  connect(scene, &SceneModel::objectRemoved, this, &SceneMachining::onObjectRemoved);
  connect(scene, &SceneModel::objectAdded, this, &SceneMachining::onObjectAdded);
  m_calculationTimer.setInterval(30);
  connect(&m_calculationTimer, &QTimer::timeout, this, &SceneMachining::finishCalculation);
  m_timer.setInterval(16);
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &SceneMachining::advancePlayback);
  connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &SceneMachining::endPlayback);
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
  const auto* edge = qobject_cast<const SceneEdgeGeometry*>(object->geometry());
  if (!edge) return std::nullopt;
  const BoundedCylinder* cylinder = nullptr;
  bool hasPlane = false;
  for (quint32 id : edge->sourceIds()) {
    const auto* source = m_scene->findObject(id);
    if (!source) return std::nullopt;
    if (source->plane()) hasPlane = true;
    if (source->cylinder()) cylinder = source->cylinder();
  }
  if (!hasPlane || !cylinder) return std::nullopt;
  ChamferPathParameters parameters;
  parameters.edge = edge->edge().ellipse();
  const auto& origin = cylinder->origin();
  const auto& axis = cylinder->axis();
  parameters.cylinderOrigin = V3d{origin[0], origin[1], origin[2]};
  parameters.cylinderAxis = V3d{axis[0], axis[1], axis[2]};
  return parameters;
}

QString SceneMachining::unavailableReason() const
{
  if (m_calculating) return tr("Calculating trajectory…");
  if (playbackActive()) return tr("Stop Dry Run before generating a trajectory.");
  if (!m_model || !m_robotAvailable) return tr("Robot preview is not ready.");
  if (m_effectors->spindleTcp().size() != 6) return tr("Apply the spindle TCP calibration first.");
  if (!sourceParameters()) return tr("Select an intersection edge with its source plane and cylinder, or a generated machining path.");
  return {};
}

std::optional<ChamferPathParameters> SceneMachining::previewParameters() const
{
  const auto* object = m_scene->findObject(m_inputId);
  if (!object || !object->visible()) return std::nullopt;
  const auto source = sourceParameters();
  return source ? std::optional<ChamferPathParameters>{m_settings.applyTo(*source)} : std::nullopt;
}

void SceneMachining::apply()
{
  if (m_calculating) return;
  const QString reason = unavailableReason();
  if (!reason.isEmpty()) { setError(reason); return; }
  const auto source = sourceParameters();
  const auto timing = m_settings.timing();
  if (!source || !timing) { setError(tr("Six joint speed and acceleration limits are required.")); return; }
  const auto path = ChamferPath::create(m_settings.applyTo(*source));
  if (!path.path) { setError(path.error); return; }
  const auto& tcp = m_effectors->spindleTcp();
  ChamferRobotSetup robot;
  robot.model = *m_model;
  robot.flangeToTcp = makeTransform(euler2rot(tcp[3], tcp[4], tcp[5]), V3d{tcp[0], tcp[1], tcp[2]});
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
  const auto* previous = m_scene->findObject(m_calculationInputId);
  if (!previous || m_inputId != m_calculationInputId || m_revision != m_calculationRevision
      || !m_robotAvailable || !sourceParameters()) {
    setError(tr("Scene or robot configuration changed. Apply again."));
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
    setError(tr("Cannot insert trajectory: scene object IDs are exhausted."));
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

void SceneMachining::setRobotModel(const RoboCrap3D::Kr10KinematicModel& model)
{
  m_model = model;
  calibrationChanged();
}

void SceneMachining::setRobotAvailable(bool available)
{
  if (m_robotAvailable == available) return;
  if (!available) endPlayback();
  m_robotAvailable = available;
  refreshCompatibility();
}

void SceneMachining::calibrationChanged()
{
  endPlayback();
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
  if (path->data().calibrationRevision != m_revision)
    reason = tr("Robot or TCP calibration changed. Regenerate this trajectory.");
  else if (!m_robotAvailable) reason = tr("Robot preview is not ready.");
  path->setIncompatibility(reason);
}

void SceneMachining::onObjectAdded(SceneObject* object)
{
  updateCompatibility(object);
  emit availabilityChanged();
}

void SceneMachining::onObjectRemoved(quint32 id)
{
  if (m_activePathId == id) endPlayback();
  if (m_inputId == id) setInputId(0);
  else emit availabilityChanged();
}

void SceneMachining::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  emit errorChanged();
}

bool SceneMachining::canPlay() const
{
  if (m_calculating || m_playing || !m_robotAvailable) return false;
  if (playbackActive()) return true;
  const auto* object = m_scene->findObject(m_inputId);
  const auto* path = object ? qobject_cast<const SceneMachiningPath*>(object->geometry()) : nullptr;
  return path && path->compatible();
}

void SceneMachining::dryRun()
{
  if (!canPlay()) { setError(tr("Select a compatible machining path before Dry Run.")); return; }
  if (!isSignalConnected(QMetaMethod::fromSignal(&SceneMachining::playbackPoseRequested))) {
    setError(tr("Robot presentation is not connected."));
    return;
  }
  if (playbackActive()) {
    m_resumeTime = m_elapsed;
    m_playing = true;
    m_clock.start();
    m_timer.start();
    emit playbackChanged();
    emit availabilityChanged();
    return;
  }
  const auto* path = qobject_cast<const SceneMachiningPath*>(m_scene->findObject(m_inputId)->geometry());
  m_activeMotion = path->data().motion;
  m_activePathId = m_inputId;
  m_duration = m_activeMotion->duration();
  m_elapsed = 0.0;
  m_resumeTime = 0.0;
  m_playing = true;
  setError({});
  emit availabilityChanged();
  if (!presentPlayback(0.0)) return;
  m_clock.start();
  m_timer.start();
}

void SceneMachining::endPlayback()
{
  if (!playbackActive()) return;
  m_timer.stop();
  m_playing = false;
  m_activeMotion.reset();
  m_activePathId = 0;
  emit playbackChanged();
  emit availabilityChanged();
}

void SceneMachining::failPlayback(const QString& error)
{
  endPlayback();
  setError(error);
}

bool SceneMachining::presentPlayback(double seconds)
{
  const auto pose = m_activeMotion->evaluate(seconds);
  if (!pose) { failPlayback(tr("Cannot evaluate the saved trajectory.")); return false; }
  QList<double> joints;
  for (double value : pose->joints) joints.append(value);
  // GUI-thread direct delivery: presentation failures stop playback before time advances.
  emit playbackPoseRequested(joints);
  if (!playbackActive()) return false;
  m_elapsed = seconds;
  switch (pose->phase) {
  case ChamferMotionPhase::Approach: m_phase = tr("HOME to P_s"); break;
  case ChamferMotionPhase::TransferIn: m_phase = tr("P_s to lead-in"); break;
  case ChamferMotionPhase::LeadIn: m_phase = tr("Lead-in"); break;
  case ChamferMotionPhase::Machining: m_phase = tr("Machining"); break;
  case ChamferMotionPhase::LeadOut: m_phase = tr("Lead-out"); break;
  case ChamferMotionPhase::TransferOut: m_phase = tr("Lead-out to P_s"); break;
  case ChamferMotionPhase::ReturnHome: m_phase = tr("Return HOME"); break;
  }
  emit playbackChanged();
  return true;
}

void SceneMachining::advancePlayback()
{
  if (!m_playing) return;
  const double seconds = std::min(m_duration, m_resumeTime + m_clock.nsecsElapsed() / 1.0e9);
  if (presentPlayback(seconds) && seconds >= m_duration) endPlayback();
}

void SceneMachining::pause()
{
  if (!m_playing) return;
  advancePlayback();
  if (!playbackActive()) return;
  m_timer.stop();
  m_playing = false;
  emit playbackChanged();
  emit availabilityChanged();
}

void SceneMachining::stop()
{
  if (!playbackActive()) return;
  m_timer.stop();
  if (m_robotAvailable) (void)presentPlayback(0.0);
  endPlayback();
}