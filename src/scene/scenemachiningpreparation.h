#pragma once

#include "scenemachining.h"
#include "pathgeneration/chamfer/chamferjob.h"

#include <QPointer>
#include <QTimer>
#include <QFileSystemWatcher>
#include <future>

class SceneMachiningPreparation final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool canPrepare READ canPrepare NOTIFY availabilityChanged)
  Q_PROPERTY(QString unavailableReason READ unavailableReason NOTIFY availabilityChanged)
  Q_PROPERTY(bool preparing READ preparing NOTIFY preparingChanged)
  Q_PROPERTY(bool prepared READ prepared NOTIFY preparedChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(QString jobId READ jobId NOTIFY preparedChanged)
  Q_PROPERTY(double duration READ duration NOTIFY preparedChanged)
  Q_PROPERTY(double centralTimeScale READ centralTimeScale NOTIFY preparedChanged)
  Q_PROPERTY(int sampleCount READ sampleCount NOTIFY preparedChanged)
  Q_PROPERTY(bool encodingReady READ encodingReady NOTIFY preparedChanged)
  Q_PROPERTY(QString encodingUnavailableReason READ encodingUnavailableReason NOTIFY preparedChanged)

public:
  // Scene lookup, selected machining input and QObject lifetime are distinct responsibilities.
  SceneMachiningPreparation(SceneModel* scene, SceneMachining* machining, QObject* parent = nullptr);
  ~SceneMachiningPreparation() override;
  bool canPrepare() const { return unavailableReason().isEmpty(); }
  QString unavailableReason() const;
  bool preparing() const { return m_preparing; }
  bool prepared() const { return bool(m_job); }
  QString error() const { return m_error; }
  QString jobId() const { return m_job ? m_job->source().jobId : QString{}; }
  double duration() const { return m_job ? m_job->duration() : 0.0; }
  double centralTimeScale() const { return m_job ? m_job->centralTimeScale() : 1.0; }
  int sampleCount() const { return m_job ? m_job->sampleCount() : 0; }
  bool encodingReady() const { return m_job && m_job->encodingReady(); }
  QString encodingUnavailableReason() const
  { return m_job ? m_job->encodingUnavailableReason() : QString{}; }
  Q_INVOKABLE void prepare();
  void shutdown();

signals:
  void availabilityChanged();
  void preparingChanged();
  void preparedChanged();
  void errorChanged();
  void preparedJobReady(PreparedChamferJobPtr job);

private:
  SceneMachiningPath* selectedPath() const;
  bool sourceIsCurrent(const ChamferJobSource& source) const;
  void observeSelectedPath();
  void onInputChanged();
  void onAvailabilityChanged();
  void onObjectRemoved(quint32 id);
  void onLimitFileChanged(const QString& path);
  void refreshLimitConfiguration();
  void finishPreparation();
  void setPreparedJob(PreparedChamferJobPtr job);
  void setError(const QString& error);

  SceneModel* m_scene;
  SceneMachining* m_machining;
  QPointer<SceneMachiningPath> m_path;
  QMetaObject::Connection m_compatibilityConnection;
  QTimer m_timer;
  QFileSystemWatcher m_limitWatcher;
  QString m_limitPath;
  RsiCorrectionLimitConfiguration m_limitConfiguration;
  std::future<PreparedChamferJobResult> m_future;
  ChamferJobSource m_pendingSource;
  PreparedChamferJobPtr m_job;
  quint64 m_generation = 0;
  quint64 m_pendingGeneration = 0;
  QString m_error;
  bool m_preparing = false;
  bool m_shuttingDown = false;
};
