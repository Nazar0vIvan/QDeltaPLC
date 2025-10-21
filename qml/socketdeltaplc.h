#ifndef SOCKETDELTAPLC_H
#define SOCKETDELTAPLC_H

#include <QTcpSocket>
#include <QThread>

#include "logger.h"

#define LOCAL_ADDRESS "192.168.2.1"
#define LOCAL_PORT 2222
#define PEER_ADDRESS "192.168.2.5"
#define PEER_PORT 3333

#include "plcmessagemanager.h"


class SocketDeltaPLC : public QTcpSocket
{
    Q_OBJECT

public:
  SocketDeltaPLC(const QString& name, QObject *parent = nullptr);
  ~SocketDeltaPLC();

  Q_INVOKABLE void connectToHost();
  Q_INVOKABLE virtual void disconnectFromHost() override;
  Q_INVOKABLE void writeMessage(const QVariantMap& msg);
  Q_INVOKABLE void setSocketConfig(const QVariantMap& config);

signals:
  void logMessage(const LoggerMessage& msg);
  void errorOccurredMessage(const LoggerMessage& msg);
  void stateChangedMessage(const LoggerMessage& msg);
  void plcDataReady(const QVariantMap& data);

public slots:
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState state);
  void onConnected();
  void onReadyRead();

private:
  bool tearDownToUnconnected(int ms = 300);
  QString stateToString(SocketState state);

  QHostAddress m_la;
  qint16 m_lp = 0;
  QHostAddress m_pa;
  qint16 m_pp = 0;

  PlcMessageParser m_parser;
};

#endif // SOCKETDELTAPLC_H
