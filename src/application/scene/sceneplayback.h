#pragma once

#include "scenemodel.h"
#include "kinematics/kr10kinematicmodel.h"
#include "trajectories/chamfermotion.h"
#include "trajectories/probetouchmotion.h"

#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>
#include <memory>
#include <optional>
#include <variant>

namespace RoboCrap3D { class RobotController; }

// An immutable saved motion, chamfer machining or probe calibration; monostate when none.
using PlaybackMotion = std::variant<std::monostate, std::shared_ptr<const ChamferMotion>,
                                    std::shared_ptr<const ProbeTouchMotion>>;

// Dry Run of one immutable saved motion, shared by the motion coordinators, plus the robot
// model and preview availability that RobotController publishes.
class ScenePlayback : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool active READ active NOTIFY stateChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY stateChanged)
  Q_PROPERTY(double elapsed READ elapsed NOTIFY playbackChanged)
  Q_PROPERTY(double duration READ duration NOTIFY stateChanged)
  Q_PROPERTY(QString phase READ phase NOTIFY playbackChanged)
public:
  explicit ScenePlayback(SceneModel* scene);
  bool active() const { return !std::holds_alternative<std::monostate>(m_motion); }
  bool playing() const { return m_playing; }
  double elapsed() const { return m_elapsed; }
  double duration() const { return m_duration; }
  QString phase() const { return m_phase; }
  // Scene object id of the path whose motion is played; 0 when nothing is active.
  quint32 ownerId() const { return m_ownerId; }
  // Presents time zero, then plays from it.
  void start(PlaybackMotion motion, quint32 ownerId);
  // Continues a paused motion from its elapsed time.
  void resume();
  Q_INVOKABLE void pause();
  // Presents time zero, which is HOME, then ends playback.
  Q_INVOKABLE void stop();
  void end();
  const std::optional<RoboCrap3D::Kr10KinematicModel>& robotModel() const { return m_model; }
  bool robotAvailable() const { return m_robotAvailable; }
  void setRobotModel(const RoboCrap3D::Kr10KinematicModel& model);
  void setRobotAvailable(bool available);
  void setRobotController(RoboCrap3D::RobotController* controller);

signals:
  // Start, resume, pause and end; playbackChanged reports every presented time.
  void stateChanged();
  void playbackChanged();
  void failed(quint32 ownerId, const QString& error);
  void robotModelChanged();
  void robotAvailabilityChanged();

private:
  void onObjectRemoved(quint32 id);
  void advance();
  bool present(double seconds);
  void fail(const QString& error);

  QPointer<RoboCrap3D::RobotController> m_robotController;
  std::optional<RoboCrap3D::Kr10KinematicModel> m_model;
  bool m_robotAvailable = false;
  QTimer m_timer;
  QElapsedTimer m_clock;
  PlaybackMotion m_motion;
  quint32 m_ownerId = 0;
  bool m_playing = false;
  double m_elapsed = 0.0;
  double m_resumeTime = 0.0;
  double m_duration = 0.0;
  QString m_phase;
};
