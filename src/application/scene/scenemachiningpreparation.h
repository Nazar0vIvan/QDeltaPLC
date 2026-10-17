#pragma once

#include "scenemachining.h"
#include "trajectories/chamferrsistream.h"

#include <QTimer>
#include <future>
#include <optional>

// RSI Sampling: turns a selected saved path into the controller correction
// stream and C:/RoboCrapJobs/shared_data.txt. The stream belongs to its path,
// not to the selection; it is dropped when that path is removed or incompatible.
class SceneMachiningPreparation final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool canPrepare READ canPrepare NOTIFY availabilityChanged)
  Q_PROPERTY(QString unavailableReason READ unavailableReason NOTIFY availabilityChanged)
  Q_PROPERTY(bool preparing READ preparing NOTIFY preparingChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(QString summary READ summary NOTIFY preparedChanged)

public:
  // Scene lookup, selected machining input and QObject lifetime are distinct responsibilities.
  SceneMachiningPreparation(SceneModel* scene, SceneMachining* machining, QObject* parent = nullptr);
  ~SceneMachiningPreparation() override;
  bool canPrepare() const { return unavailableReason().isEmpty(); }
  QString unavailableReason() const;
  bool preparing() const { return m_preparing; }
  QString error() const { return m_error; }
  QString summary() const; // Empty while no stream is held.
  Q_INVOKABLE void prepare();
  void shutdown();

signals:
  void availabilityChanged();
  void preparingChanged();
  void preparedChanged();
  void errorChanged();
  // Empty when the stream is cleared; implicitly shared, never mutated.
  void correctionsReady(const QVector<V6d>& increments);

private:
  struct PreparedRsiStream
  {
    quint32 pathId = 0;
    QString pathName;
    std::shared_ptr<const ChamferMotion> motion; // Identity of the sampled saved motion.
    ChamferRsiStream stream;
  };

  SceneMachiningPath* pathById(quint32 objectId) const;
  bool isCurrent(const PreparedRsiStream& prepared) const;
  void onAvailabilityChanged();
  void finishPreparation();
  void clearPrepared();
  void setError(const QString& error);

  SceneModel* m_scene;
  SceneMachining* m_machining;
  QTimer m_timer;
  std::future<ChamferRsiStreamResult> m_future;
  PreparedRsiStream m_pending; // Input of the running worker; its stream is unused.
  std::optional<PreparedRsiStream> m_prepared;
  QString m_error;
  bool m_preparing = false;
  bool m_shuttingDown = false;
};
