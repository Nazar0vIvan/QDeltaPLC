#pragma once

#include "devices/abstractdevice.h"
#include "devices/plc/plcmessagemanager.h"

#include <QByteArray>
#include <QHostAddress>
#include <QHash>
#include <QQueue>
#include <QVariantList>

class QFile;
class QTcpSocket;
class QTimer;

class PlcDevice : public AbstractDevice
{
  Q_OBJECT

public:
  explicit PlcDevice(const QString& name, QObject* parent = nullptr);
  ~PlcDevice() override = default;

  Q_INVOKABLE void connect(const QVariantMap& config);
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void setOutput(const QVariantMap& args);
  Q_INVOKABLE void writeMessage(const QVariantMap& msg);
  Q_INVOKABLE void runRobotProgram(const QVariantMap& args);
  Q_INVOKABLE void resumeRobot();
  // Aborts only the PLC start sequence; a running KRL program is not stopped.
  Q_INVOKABLE void abortRobotStart();

protected:
  void startDevice() override;
  void stopDevice() override;

private slots:
  void onConnected();
  void onReadyRead();
  void onStateChanged(QAbstractSocket::SocketState state);
  void onReplyTimeout();
  void onHeartbeat();

private:
  // KUKA application program state derived from the X1 status bits, published as robotProgramState.
  enum class ProgramState : quint8 { Unknown = 0, Running = 1, Finished = 2, Interrupted = 3 };

  static QByteArray swapBytes(const QByteArray& data);
  static ProgramState nextProgramState(ProgramState current, const QVariantList& x1);
  void sendNext();
  bool matchResponse(const QVariantMap& data);
  void processIncoming(const QByteArray& bytes);

  void publishData(const QVariantMap& data);
  void publishIo(const QVariantMap& data);
  // records/plc_io_<time>.csv, one file per connection.
  void openLog();
  void writeLog(const QString& event);

  QTcpSocket* m_sock = nullptr;
  PlcMessageManager* m_mgr = nullptr;
  QTimer* m_replyTimer = nullptr;
  QTimer* m_heartbeat = nullptr;
  QFile* m_log = nullptr;
  // Last logged x1/y1 bits and robot step/program/result: every log row shows the full state.
  QVariantMap m_logged;

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  QByteArray m_rx;
  QQueue<QVariantMap> m_queue;
  QHash<quint8, QVariantMap> m_pend;
  quint8 m_nextTid = 1;
  ProgramState m_programState = ProgramState::Unknown;
  // Mirror of the PLC's ResumeBlocked, published as robotResumeBlocked.
  bool m_resumeBlocked = false;
};
