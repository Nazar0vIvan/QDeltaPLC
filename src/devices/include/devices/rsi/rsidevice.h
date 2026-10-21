#pragma once

#include "devices/abstractdevice.h"
#include "geometry/mathtypes.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QVector>

#include <future>
#include <vector>

class QNetworkDatagram;
class QTimer;
class QUdpSocket;

// KUKA RSI UDP partner for RSI_Chamfer_Hole: answers every controller cycle and plays
// the sampled correction stream while the controller reports Start = 1.
class RsiDevice : public AbstractDevice
{
  Q_OBJECT

public:
  // One PC reply. Stop ends RSI_MOVECORR; Result = 1 reports normal completion.
  struct Reply
  {
    quint64 ipoc = 0;
    V6d correction = V6d::Zero(); // Relative X/Y/Z mm, A/B/C deg.
    bool stop = false;
    qint64 result = 0;
  };

  explicit RsiDevice(const QString& name, QObject* parent = nullptr);

  Q_INVOKABLE void connect(const QVariantMap& config);
  Q_INVOKABLE void disconnect();

public slots:
  // Discard buffered requests and reset run/peer state; install at index 0 in Idle.
  void setCorrections(const QVector<V6d>& increments, const QString& diagnosticId);

protected:
  void startDevice() override;
  void stopDevice() override;

private:
  enum class RunState { Idle, Streaming, Finished, Stopping };

  struct CorrectionStream
  {
    QVector<V6d> increments;
    QString diagnosticId;
  };

  void onReadyRead();
  bool acceptsSender(const QNetworkDatagram& datagram) const;
  // State-machine step for the newest accepted request.
  Reply advance(bool start, qint64 echoedResult);
  void resetRun();
  void setState(RunState state);
  void publishState();
  void appendTrace(const QByteArray& row);
  void saveTrace();
  static QString writeTrace(QByteArray rows, QString fileName);

  QUdpSocket* m_sock = nullptr;
  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0; // Learned from the first accepted controller packet.

  CorrectionStream m_corrections;
  qsizetype m_index = 0;
  RunState m_state = RunState::Idle;
  QElapsedTimer m_traceClock;
  QTimer* m_traceTimer = nullptr;
  QByteArray m_trace;
  qsizetype m_traceDropped = 0;
  int m_tracePart = 0;
  std::vector<std::future<QString>> m_traceSaves;
};
