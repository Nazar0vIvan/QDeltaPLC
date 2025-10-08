#ifndef SOCKETRUNNER_H
#define SOCKETRUNNER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QVector>
#include <QPointF>
#include <QTimer>

#include "logger.h"

class AbstractSocketRunner : public QObject
{
    Q_OBJECT

public:
    explicit AbstractSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
    ~AbstractSocketRunner() override;

    Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)
    Q_INVOKABLE void setSocketConfig(const QVariantMap& config);

    int socketState() const { return m_socketState; }

signals:
    void logMessage(const LoggerMessage& msg);
    void bufferChanged();
    void socketStateChanged();

public slots:
    void start();
    void stop(); // connects to aboutToQuit app signal

    void onSocketStateChanged(QAbstractSocket::SocketState state);

    void onThreadStarted();
    void onThreadFinished();

protected:
    QAbstractSocket* m_socket = nullptr;
    QThread* m_thread = nullptr;

private:
    void attachSocket(QAbstractSocket* sock);

    int m_socketState = QAbstractSocket::UnconnectedState;
    QVariant m_buffer;
};

class TcpSocketRunner : public AbstractSocketRunner
{
    Q_OBJECT

public:
    explicit TcpSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
    ~TcpSocketRunner() override;

    Q_INVOKABLE void connectToHost();
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void writeMessage(const QVariantMap& cmd);
};

class UdpSocketRunner : public AbstractSocketRunner
{
    Q_OBJECT

public:
    explicit UdpSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
    ~UdpSocketRunner() override;

    Q_PROPERTY(QVariantList lastReading READ lastReading NOTIFY lastReadingChanged)
    Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY isStreamingChanged)

    Q_INVOKABLE void startStreaming();
    Q_INVOKABLE void stopStreaming();

    QVariantList lastReading() const { return m_lastReading; }
    bool isStreaming() const { return m_isStreaming; }

signals:
    void lastReadingChanged();
    void isStreamingChanged();

public slots:
    void onBufferReady(const QVector<QVariantList>& readings);

private slots:
    void onPulse();

private:
    QVariantList m_lastReading = {};
    bool m_isStreaming = false;
    QTimer m_timer;
};

#endif // SOCKETRUNNER_H
