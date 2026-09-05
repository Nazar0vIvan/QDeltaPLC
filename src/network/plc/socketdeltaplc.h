#pragma once

#include <QTcpSocket>
#include <QThread>
#include <QVariant>

#include "logger.h"
#include "plcmessagemanager.h"

class SocketDeltaPLC : public QTcpSocket
{
  Q_OBJECT

public:
  enum CELL_STATE : quint8 {
    IDLE = 0xD0,
    RUN  = 0xD3,
    FIN  = 0xDC
  };
  Q_ENUM(CELL_STATE)

  SocketDeltaPLC(const QString& name, QObject *parent = nullptr);
  ~SocketDeltaPLC();

  Q_INVOKABLE void connect(const QVariantMap& config);
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void writeMessage(const QVariantMap& msg);

signals:
  void logMessage(const LoggerMessage& msg);
  void errorOccurredMessage(const LoggerMessage& msg);
  void stateChangedMessage(const LoggerMessage& msg);
  void dataReady(const QVariantMap& data);

public slots:
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState state);
  void onConnected();
  void onReadyRead();

private:
  bool tearDownToUnconnected(int ms = 300);
  QString stateToString(SocketState state);
  QByteArray swapBytes(const QByteArray& data);

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  PlcMessageManager m_mgr;
  quint8 m_nextTid = 1;


};

