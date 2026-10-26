#pragma once

#include "scenemodel.h"
#include "sceneendeffectors.h"
#include "sceneplayback.h"
#include "viewport/occcontroller.h"
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
  Q_PROPERTY(bool canPlay READ canPlay NOTIFY availabilityChanged)
public:
  SceneMachining(SceneModel* scene, SceneEndEffectors* effectors, ScenePlayback* playback);
  SceneMachiningSettings settings() const { return m_settings; }
  void setSettings(const SceneMachiningSettings& settings);
  quint32 inputId() const { return m_inputId; }
  void setInputId(quint32 id);
  bool canGenerate() const { return unavailableReason().isEmpty(); }
  QString unavailableReason() const;
  QString error() const { return m_error; }
  Q_INVOKABLE void generate();
  bool calculating() const { return m_calculating; }
  Q_INVOKABLE void dryRun();
  // Stops the playback only while a machining path owns it.
  void stop();
  bool canPlay() const;

signals:
  void draftAxisChanged(const std::optional<RoboCrap3D::DrawingAxis>& axis);
  void settingsChanged();
  void inputChanged();
  void availabilityChanged();
  void errorChanged();
  void generated(quint32 objectId);
  void calculatingChanged();

private:
  std::optional<ChamferPathParameters> sourceParameters() const;
  void synchronizeDraftAxis();
  const SceneMachiningPath* machiningPath(quint32 id) const;
  bool ownsPlayback() const;
  void applyRobotModel();
  void robotSetupChanged();
  void refreshCompatibility();
  void onObjectAdded(SceneObject* object);
  void updateCompatibility(SceneObject* object);
  void onObjectRemoved(quint32 id);
  void setError(const QString& error);
  void onPlaybackFailed(quint32 ownerId, const QString& error);
  void finishCalculation();
  void setCalculating(bool calculating);

  SceneModel* m_scene;
  SceneEndEffectors* m_effectors;
  ScenePlayback* m_playback;
  SceneMachiningSettings m_settings;
  quint64 m_revision = 0;
  quint32 m_inputId = 0;
  QString m_error;
  QTimer m_calculationTimer;
  std::future<ChamferMotionResult> m_calculation;
  quint32 m_calculationInputId = 0;
  quint64 m_calculationRevision = 0;
  bool m_calculating = false;
};
