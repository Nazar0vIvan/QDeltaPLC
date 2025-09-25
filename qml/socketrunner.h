#ifndef SOCKETRUNNER_H
#define SOCKETRUNNER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>

#include "logger.h"

class AbstractSocketRunner : public QObject
{
    Q_OBJECT

public:
    explicit AbstractSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
    ~AbstractSocketRunner() override;

    Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)
    Q_PROPERTY(bool isReading READ isReading NOTIFY isReadingChanged)
    Q_PROPERTY(QVariant buffer READ buffer NOTIFY bufferChanged)

    int socketState() const { return m_socketState; }
    QVariant buffer() const { return m_buffer; }
    QVariant isReading() const { return m_isReading; }

    void start();

signals:
    void logMessage(const LoggerMessage& msg);
    void bufferChanged();
    void socketStateChanged();
    void isReadingChanged();

public slots:
    void stop();

    void slotThreadStarted();
    void slotThreadFinished();

protected:
    QAbstractSocket* m_socket = nullptr;

private:
    void attachSocket(QAbstractSocket* sock);

    QThread* m_thread = nullptr;

    int m_socketState = QAbstractSocket::UnconnectedState;
    QVariant m_buffer;
    bool m_isReading = false;
};

class TcpSocketRunner : public AbstractSocketRunner
{
    Q_OBJECT

public:
    explicit TcpSocketRunner(QObject* parent = nullptr);
    ~TcpSocketRunner() override;

    Q_INVOKABLE void connectToHost(const QVariantMap& data);
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void writeMessage(const QString& msg);
};

class UdpSocketRunner : public AbstractSocketRunner
{
    Q_OBJECT

public:
    explicit UdpSocketRunner(QObject* parent = nullptr);
    ~UdpSocketRunner() override;

    Q_INVOKABLE void startStreaming(const QVariantMap& data);
    Q_INVOKABLE void stopStreaming();
};




#endif // SOCKETRUNNER_H
