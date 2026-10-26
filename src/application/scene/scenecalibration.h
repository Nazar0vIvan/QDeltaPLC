#pragma once

#include "scenemodel.h"
#include "sceneendeffectors.h"
#include "sceneplayback.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <future>

// Probe sphere calibration in the Measuring/Calibration submode: the sphere from the deployed
// sphere file, the generated calibration path and the visibility of the other scene objects
// while the submode is active.
class SceneCalibration : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(bool canGenerate READ canGenerate NOTIFY availabilityChanged)
  Q_PROPERTY(QString unavailableReason READ unavailableReason NOTIFY availabilityChanged)
  Q_PROPERTY(bool calculating READ calculating NOTIFY availabilityChanged)
  Q_PROPERTY(bool canPlay READ canPlay NOTIFY availabilityChanged)
public:
  SceneCalibration(SceneModel* scene, SceneEndEffectors* effectors, ScenePlayback* playback);
  QString error() const { return m_error; }
  // Entering re-reads the sphere file and hides every other object; leaving restores them.
  void setActive(bool active);
  bool canGenerate() const { return unavailableReason().isEmpty(); }
  QString unavailableReason() const;
  bool calculating() const { return m_calculation.valid(); }
  Q_INVOKABLE void generate();
  bool canPlay() const;
  // Resumes the paused calibration playback or starts the calibration path from HOME.
  Q_INVOKABLE void dryRun();

signals:
  void errorChanged();
  void availabilityChanged();

private:
  void activate();
  void deactivate();
  const SceneSphere* sphere() const;
  const SceneTouchPath* path() const;
  bool ownsPlayback() const;
  void onPlaybackFailed(quint32 ownerId, const QString& error);
  void replaceSphere(const SceneSphere& sphere);
  // A changed sphere, measuring TCP or robot model makes the calibration path invalid.
  void invalidatePath();
  void finishCalculation();
  void hideOther(SceneObject* object);
  void onObjectAdded(SceneObject* object);
  void setError(const QString& error);

  SceneModel* m_scene;
  SceneEndEffectors* m_effectors;
  ScenePlayback* m_playback;
  bool m_active = false;
  quint32 m_sphereId = 0;
  quint32 m_pathId = 0;
  QList<quint32> m_hiddenIds; // Objects hidden on entry, shown again on exit.
  QString m_error;
  quint64 m_revision = 0; // Increased by invalidatePath.
  quint64 m_calculationRevision = 0;
  QTimer m_calculationTimer;
  std::future<ProbeTouchMotionResult> m_calculation;
};
