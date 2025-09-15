#ifndef SOCKETRUNNER_H
#define SOCKETRUNNER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>

#include "logger.h"

class SocketRunner : public QObject
{
    Q_OBJECT

public:
    explicit SocketRunner(QTcpSocket* existingSocket, QObject* parent = nullptr);
    ~SocketRunner() override;

    Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)

    Q_INVOKABLE void connectToHost(const QVariantMap& data);
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void writeMessage(const QString& msg);

    QObject* socket() const { return m_socket; }
    int  socketState() const { return m_socketState; }

signals:
    void threadStartedMessage(const LoggerMessage& msg);
    void threadFinishedMessage(const LoggerMessage& msg);
    void socketStateChanged();

public slots:
    void start();
    void stop(bool deleteSocket = false);

private:
    void attachSocket(QTcpSocket* sock);

    QThread*    m_thread = nullptr;
    QTcpSocket* m_socket = nullptr;
    int         m_socketState = QAbstractSocket::UnconnectedState;
};

#endif // SOCKETRUNNER_H
