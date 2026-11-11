#pragma once

#include "scenecalibration.h"
#include "scenemachining.h"
#include "trajectories/chamferrsistream.h"
#include "trajectories/probersistream.h"

#include <QTimer>
#include <future>
#include <memory>
#include <optional>

// RSI Sampling: turns a selected saved machining path into the controller correction stream and
// C:/RoboCrapJobs/shared_data.txt, or the calibration path into one search stream per touch and
// C:/RoboCrapJobs/calibration_data.txt. At most one stream table and its job file are held; they
// belong to the sampled path, not the selection, and are dropped when that path is removed or
// incompatible.
class SceneMachiningPreparation final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool canPrepare READ canPrepare NOTIFY availabilityChanged)
  Q_PROPERTY(QString unavailableReason READ unavailableReason NOTIFY availabilityChanged)
  Q_PROPERTY(bool canPrepareCalibration READ canPrepareCalibration NOTIFY availabilityChanged)
  Q_PROPERTY(QString calibrationUnavailableReason READ calibrationUnavailableReason NOTIFY availabilityChanged)
  Q_PROPERTY(bool preparing READ preparing NOTIFY preparingChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(QString summary READ summary NOTIFY preparedChanged)

public:
  SceneMachiningPreparation(SceneModel* scene, SceneMachining* machining, SceneCalibration* calibration);
  ~SceneMachiningPreparation() override;
  bool canPrepare() const { return unavailableReason().isEmpty(); }
  QString unavailableReason() const;
  bool canPrepareCalibration() const { return calibrationUnavailableReason().isEmpty(); }
  QString calibrationUnavailableReason() const;
  bool preparing() const { return m_preparing; }
  QString error() const { return m_error; }
  QString summary() const; // Empty while no stream is held.
  Q_INVOKABLE void prepare();
  // Samples the generated calibration path for RSI_Probe_Calibration.
  Q_INVOKABLE void prepareCalibration();
  // Hard-coded RSI_Probe_Check stream: WORLD +X at 1 mm/s for 100 mm; replaces a held stream and file.
  Q_INVOKABLE void prepareTestLin();
  void shutdown();

signals:
  void availabilityChanged();
  void preparingChanged();
  void preparedChanged();
  void errorChanged();
  // Empty values clear the streams; the table (stream i for TouchIndex i) and its log ID travel together.
  void correctionsReady(const QVector<QVector<V6d>>& streams, const QString& diagnosticId);

private:
  struct PreparedRsiStream
  {
    quint32 pathId = 0;
    QString pathName;
    std::shared_ptr<const ChamferMotion> motion; // Identity of the sampled saved motion.
    ChamferRsiStream stream;
  };

  struct PreparedCalibration
  {
    std::shared_ptr<const ProbeTouchMotion> motion; // Identity of the sampled calibration path.
    int touchCount = 0;
    qsizetype stepCount = 0;
  };

  SceneMachiningPath* pathById(quint32 objectId) const;
  bool isCurrent(const PreparedRsiStream& prepared) const;
  bool isCurrentCalibration(const std::shared_ptr<const ProbeTouchMotion>& motion) const;
  void onAvailabilityChanged();
  void onCalibrationAvailabilityChanged();
  void finishPreparation();
  void finishCalibration();
  void clearPrepared();
  void setError(const QString& error);

  SceneModel* m_scene;
  SceneMachining* m_machining;
  SceneCalibration* m_calibration;
  QTimer m_timer;
  std::future<ChamferRsiStreamResult> m_future;
  PreparedRsiStream m_pending; // Input of the running worker; its stream is unused.
  std::optional<PreparedRsiStream> m_prepared;
  std::future<ProbeCalibrationRsiResult> m_calibrationFuture;
  std::shared_ptr<const ProbeTouchMotion> m_pendingCalibration; // Input of the running calibration worker.
  std::optional<PreparedCalibration> m_preparedCalibration;
  QString m_error;
  bool m_preparing = false;
  bool m_testLin = false; // The installed stream is the test LIN, not a sampled path.
  bool m_shuttingDown = false;
};
