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

    QObject* socket() const { return m_socket; }

    Q_INVOKABLE void setConfig(const QVariantMap& data);
    Q_INVOKABLE void connectToHost();
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void writeMessage(const QVariantMap& msg);

signals:
    void threadStartedMessage(const LoggerMessage& msg);
    void threadFinishedMessage(const LoggerMessage& msg);

public slots:
    void start();
    void stop(bool deleteSocket = false);

private:
    void attachSocket(QTcpSocket* sock);

    QThread*    m_thread = nullptr;
    QTcpSocket* m_socket = nullptr;
};

#endif // SOCKETRUNNER_H
