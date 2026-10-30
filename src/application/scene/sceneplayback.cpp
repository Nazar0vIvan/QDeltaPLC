#include "sceneplayback.h"
#include "robot/robotcontroller.h"

#include <QCoreApplication>
#include <algorithm>
#include <utility>

ScenePlayback::ScenePlayback(SceneModel* scene)
{
  connect(scene, &SceneModel::objectRemoved, this, &ScenePlayback::onObjectRemoved);
  m_timer.setInterval(16);
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &ScenePlayback::advance);
  connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &ScenePlayback::end);
}

void ScenePlayback::start(std::shared_ptr<const TimedMotion> motion, quint32 ownerId)
{
  if (!m_robotController) { emit failed(ownerId, tr("Robot presentation is not connected.")); return; }
  m_motion = std::move(motion);
  m_ownerId = ownerId;
  m_duration = m_motion->duration();
  m_elapsed = 0.0;
  m_resumeTime = 0.0;
  m_playing = true;
  emit stateChanged();
  if (!present(0.0)) return;
  m_clock.start();
  m_timer.start();
}

void ScenePlayback::resume()
{
  if (!active() || m_playing) return;
  if (!m_robotController) { emit failed(m_ownerId, tr("Robot presentation is not connected.")); return; }
  m_resumeTime = m_elapsed;
  m_playing = true;
  m_clock.start();
  m_timer.start();
  emit stateChanged();
}

void ScenePlayback::end()
{
  if (!active()) return;
  m_timer.stop();
  m_playing = false;
  m_motion.reset();
  m_ownerId = 0;
  emit stateChanged();
}

void ScenePlayback::fail(const QString& error)
{
  const quint32 owner = m_ownerId;
  end();
  emit failed(owner, error);
}

bool ScenePlayback::present(double seconds)
{
  if (!m_robotController) { fail(tr("Robot presentation is not connected.")); return false; }
  // Readiness/pose notifications may release the active motion during the returned operation.
  const auto motion = m_motion;
  const auto pose = motion->evaluateJoints(seconds);
  if (!pose) { fail(tr("Cannot evaluate the saved trajectory.")); return false; }
  const auto& [joints, phase] = *pose;
  const auto result = m_robotController->applyPlayback(joints, motion->robot().flangeToTcp);
  if (!result.accepted) { fail(result.error); return false; }
  if (!active()) return false;
  m_elapsed = seconds;
  m_phase = motion->phases()[phase].label;
  emit playbackChanged();
  return true;
}

void ScenePlayback::advance()
{
  if (!m_playing) return;
  const double seconds = std::min(m_duration, m_resumeTime + m_clock.nsecsElapsed() / 1.0e9);
  if (present(seconds) && seconds >= m_duration) end();
}

void ScenePlayback::pause()
{
  if (!m_playing) return;
  advance();
  if (!active()) return;
  m_timer.stop();
  m_playing = false;
  emit stateChanged();
}

void ScenePlayback::stop()
{
  if (!active()) return;
  m_timer.stop();
  if (m_robotAvailable) (void)present(0.0);
  end();
}

void ScenePlayback::onObjectRemoved(quint32 id)
{
  if (m_ownerId == id) end();
}

void ScenePlayback::setRobotModel(const RoboCrap3D::Kr10KinematicModel& model)
{
  m_model = model;
  emit robotModelChanged();
}

void ScenePlayback::setRobotAvailable(bool available)
{
  if (m_robotAvailable == available) return;
  if (!available) end();
  m_robotAvailable = available;
  emit robotAvailabilityChanged();
}

void ScenePlayback::setRobotController(RoboCrap3D::RobotController* controller)
{
  Q_ASSERT(!controller || controller->thread() == thread());
  m_robotController = controller;
}
