#pragma once

#include "scenemodel.h"
#include "sceneendeffectors.h"
#include <QElapsedTimer>
#include <QTimer>
#include <future>

class SceneMachining : public QObject
{
  Q_OBJECT
  Q_PROPERTY(SceneMachiningSettings settings READ settings WRITE setSettings NOTIFY settingsChanged)
  Q_PROPERTY(quint32 inputId READ inputId WRITE setInputId NOTIFY inputChanged)
  Q_PROPERTY(bool canGenerate READ canGenerate NOTIFY availabilityChanged)
  Q_PROPERTY(QString unavailableReason READ unavailableReason NOTIFY availabilityChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(bool calculating READ calculating NOTIFY calculatingChanged)
  Q_PROPERTY(bool playbackActive READ playbackActive NOTIFY playbackChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY playbackChanged)
  Q_PROPERTY(bool canPlay READ canPlay NOTIFY availabilityChanged)
  Q_PROPERTY(double elapsed READ elapsed NOTIFY playbackChanged)
  Q_PROPERTY(double duration READ duration NOTIFY playbackChanged)
  Q_PROPERTY(QString phase READ phase NOTIFY playbackChanged)
public:
  // Scene ownership, tool calibration and QObject lifetime define the coordinator.
  SceneMachining(SceneModel* scene, SceneEndEffectors* effectors, QObject* parent = nullptr);
  SceneMachiningSettings settings() const { return m_settings; }
  void setSettings(const SceneMachiningSettings& settings);
  quint32 inputId() const { return m_inputId; }
  void setInputId(quint32 id);
  bool canGenerate() const { return unavailableReason().isEmpty(); }
  QString unavailableReason() const;
  QString error() const { return m_error; }
  Q_INVOKABLE void apply();
  bool calculating() const { return m_calculating; }
  Q_INVOKABLE void dryRun();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void pause();
  bool playbackActive() const { return bool(m_activeMotion); }
  bool playing() const { return m_playing; }
  bool canPlay() const;
  double elapsed() const { return m_elapsed; }
  double duration() const { return m_duration; }
  QString phase() const { return m_phase; }
  void failPlayback(const QString& error);
  void setRobotModel(const RoboCrap3D::Kr10KinematicModel& model);
  void setRobotAvailable(bool available);
  std::optional<ChamferPathParameters> previewParameters() const;

signals:
  void settingsChanged();
  void inputChanged();
  void availabilityChanged();
  void errorChanged();
  void generated(quint32 objectId);
  void calculatingChanged();
  void playbackChanged();
  void playbackPoseRequested(QList<double> joints);

private:
  std::optional<ChamferPathParameters> sourceParameters() const;
  void calibrationChanged();
  void refreshCompatibility();
  void onObjectAdded(SceneObject* object);
  void updateCompatibility(SceneObject* object);
  void onObjectRemoved(quint32 id);
  void setError(const QString& error);
  void advancePlayback();
  void endPlayback();
  bool presentPlayback(double seconds);
  void finishCalculation();
  void setCalculating(bool calculating);

  SceneModel* m_scene;
  SceneEndEffectors* m_effectors;
  SceneMachiningSettings m_settings;
  std::optional<RoboCrap3D::Kr10KinematicModel> m_model;
  quint64 m_revision = 0;
  quint32 m_inputId = 0;
  bool m_robotAvailable = false;
  QString m_error;
  QTimer m_timer;
  QElapsedTimer m_clock;
  std::shared_ptr<const ChamferMotion> m_activeMotion;
  quint32 m_activePathId = 0;
  bool m_playing = false;
  double m_elapsed = 0.0;
  double m_resumeTime = 0.0;
  double m_duration = 0.0;
  QString m_phase;
  QTimer m_calculationTimer;
  std::future<ChamferMotionResult> m_calculation;
  quint32 m_calculationInputId = 0;
  quint64 m_calculationRevision = 0;
  bool m_calculating = false;
};
