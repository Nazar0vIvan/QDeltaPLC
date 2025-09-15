#ifndef DELTAPLCSOCKET_H
#define DELTAPLCSOCKET_H

#include <QTcpSocket>
#include <QThread>

#include "logger.h"

#define LOCAL_ADDRESS "192.168.2.1"
#define LOCAL_PORT 2222
#define PEER_ADDRESS "192.168.2.5"
#define PEER_PORT 3333


class DeltaPLCSocket : public QTcpSocket
{
    Q_OBJECT
public:
    DeltaPLCSocket(const QString& name, QObject *parent = nullptr);
    ~DeltaPLCSocket();

    Q_INVOKABLE void connectToHost(const QVariantMap& data);
    Q_INVOKABLE virtual void disconnectFromHost() override;
    Q_INVOKABLE void writeMessage(const QString& msg);

signals:
    void logMessage(const LoggerMessage& msg);
    void errorOccurredMessage(const LoggerMessage& msg);
    void stateChangedMessage(const LoggerMessage& msg);

    void destroyedMessage(const LoggerMessage& msg);

public slots:
    void slotErrorOccurred(QAbstractSocket::SocketError socketError);
    void slotStateChanged(QAbstractSocket::SocketState state);
    void slotConnectedMessage();
    void slotReadyRead();

private:
    bool tearDownToUnconnected(int ms = 300);
    QString stateToString(SocketState state);

    QString hostName;
};

#endif // DELTAPLCSOCKET_H
