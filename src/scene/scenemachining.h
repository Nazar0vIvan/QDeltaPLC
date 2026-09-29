#pragma once

#include "scenemodel.h"
#include "sceneendeffectors.h"
#include <QElapsedTimer>
#include <QTimer>

class SceneMachining : public QObject
{
  Q_OBJECT
  Q_PROPERTY(SceneMachiningSettings settings READ settings WRITE setSettings NOTIFY settingsChanged)
  Q_PROPERTY(quint32 inputId READ inputId WRITE setInputId NOTIFY inputChanged)
  Q_PROPERTY(bool canGenerate READ canGenerate NOTIFY availabilityChanged)
  Q_PROPERTY(QString unavailableReason READ unavailableReason NOTIFY availabilityChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY playbackChanged)
  Q_PROPERTY(bool canPlay READ canPlay NOTIFY availabilityChanged)
  Q_PROPERTY(double elapsed READ elapsed NOTIFY playbackChanged)
  Q_PROPERTY(double duration READ duration NOTIFY playbackChanged)
  Q_PROPERTY(QString phase READ phase NOTIFY playbackChanged)
  Q_PROPERTY(bool showFrames READ showFrames WRITE setShowFrames NOTIFY framePreviewChanged)
  Q_PROPERTY(int framePhase READ framePhase WRITE setFramePhase NOTIFY framePreviewChanged)
  Q_PROPERTY(double frameProgress READ frameProgress WRITE setFrameProgress NOTIFY framePreviewChanged)
  Q_PROPERTY(bool localFrame READ localFrame WRITE setLocalFrame NOTIFY framePreviewChanged)
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
  Q_INVOKABLE quint32 apply();
  Q_INVOKABLE void dryRun();
  Q_INVOKABLE void stop();
  bool playing() const { return m_playing; }
  bool canPlay() const;
  double elapsed() const { return m_elapsed; }
  double duration() const { return m_duration; }
  QString phase() const { return m_phase; }
  void failPlayback(const QString& error);
  void setRobotModel(const RoboCrap3D::Kr10KinematicModel& model);
  void setRobotAvailable(bool available);
  std::optional<ChamferPathParameters> previewParameters() const;
  QVector<SceneCoordinateFrame> coordinateFrames() const;
  bool showFrames() const { return m_showFrames; }
  void setShowFrames(bool visible);
  int framePhase() const { return m_framePhase; }
  void setFramePhase(int phase);
  double frameProgress() const { return m_frameProgress; }
  void setFrameProgress(double progress);
  bool localFrame() const { return m_localFrame; }
  void setLocalFrame(bool local);
  Q_INVOKABLE bool previewGeometry();

signals:
  void settingsChanged();
  void framePreviewChanged();
  void inputChanged();
  void availabilityChanged();
  void errorChanged();
  void generated(quint32 objectId);
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
  bool presentPlayback(double seconds);

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
  double m_duration = 0.0;
  QString m_phase;
  bool m_showFrames = false;
  int m_framePhase = 1;
  double m_frameProgress = 0.0;
  bool m_localFrame = false;
};
