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
    explicit SocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
    ~SocketRunner() override;

    Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)

    Q_INVOKABLE void connectToHost(const QVariantMap& data);
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void writeMessage(const QString& msg);

    int socketState() const { return m_socketState; }
    void start();

signals:
    void logMessage(const LoggerMessage& msg);
    void socketStateChanged();

public slots:
    void stop();

    void slotThreadStarted();
    void slotThreadFinished();

private:
    void attachSocket(QAbstractSocket* sock);

    QThread*    m_thread = nullptr;
    QAbstractSocket* m_socket = nullptr;
    int         m_socketState = QAbstractSocket::UnconnectedState;
};

#endif // SOCKETRUNNER_H
