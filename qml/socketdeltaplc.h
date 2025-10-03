#ifndef SOCKETDELTAPLC_H
#define SOCKETDELTAPLC_H

#include <QTcpSocket>
#include <QThread>

#include "logger.h"

#define LOCAL_ADDRESS "192.168.2.1"
#define LOCAL_PORT 2222
#define PEER_ADDRESS "192.168.2.5"
#define PEER_PORT 3333

/*
  COMMAND:
    cmd_id - payload
    [byte]   var]
*/


class SocketDeltaPLC : public QTcpSocket
{
    Q_OBJECT
public:
    SocketDeltaPLC(const QString& name, QObject *parent = nullptr);
    ~SocketDeltaPLC();

    Q_INVOKABLE void connectToHost(const QVariantMap& data);
    Q_INVOKABLE virtual void disconnectFromHost() override;
    Q_INVOKABLE void writeMessage(const QVariantMap& cmd);

signals:
    void logMessage(const LoggerMessage& msg);
    void errorOccurredMessage(const LoggerMessage& msg);
    void stateChangedMessage(const LoggerMessage& msg);
    void destroyedMessage(const LoggerMessage& msg);
    void plcRunning(bool isRunning);

public slots:
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
    void onStateChanged(QAbstractSocket::SocketState state);
    void onConnected();
    void onReadyRead();

private:
    bool tearDownToUnconnected(int ms = 300);
    QString stateToString(SocketState state);
};

#endif // SOCKETDELTAPLC_H
