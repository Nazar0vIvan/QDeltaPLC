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

// KUKA RSI UDP partner: answers every controller cycle and, while the controller reports
// Start = 1, plays the correction stream selected by the request's TouchIndex.
class RsiDevice : public AbstractDevice
{
  Q_OBJECT

public:
  // Fields of a controller <Rob> packet; other elements are ignored.
  struct Request
  {
    quint64 ipoc = 0;
    bool start = false;
    qint64 result = 0;
    qsizetype touchIndex = 0; // Stream index; 0 when the context leaves TouchIndex unconnected.
  };

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
  // Discard buffered requests and reset run/peer state; install the table in Idle.
  // Stream i plays for TouchIndex i; RSI_Chamfer_Hole and RSI_Probe_Check use stream 0.
  void setCorrections(const QVector<QVector<V6d>>& streams, const QString& diagnosticId);

protected:
  void startDevice() override;
  void stopDevice() override;

private:
  enum class RunState { Idle, Streaming, Finished, Stopping };

  void onReadyRead();
  void onDispatcherAboutToBlock();
  void onDispatcherAwake();
  bool acceptsSender(const QNetworkDatagram& datagram) const;
  // State-machine step for the newest accepted request.
  Reply advance(const Request& request);
  qsizetype activeStreamSize() const; // 0 when the active index has no stream.
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

  QVector<QVector<V6d>> m_streams;
  QString m_diagnosticId; // Identifies the table in packet-capture file names.
  qsizetype m_activeStream = 0; // TouchIndex of the current run.
  qsizetype m_index = 0;
  RunState m_state = RunState::Idle;
  QElapsedTimer m_traceClock;
  // Capture timing, m_traceClock ns; -1 until first recorded.
  qint64 m_sleepNs = -1; // ControlIO event loop last went to sleep.
  qint64 m_wakeNs = -1; // ControlIO event loop last woke up.
  qint64 m_handlerDoneNs = -1; // Previous replying onReadyRead finished.
  QTimer* m_traceTimer = nullptr;
  QByteArray m_trace;
  qsizetype m_traceDropped = 0;
  int m_tracePart = 0;
  std::vector<std::future<QString>> m_traceSaves;
};
