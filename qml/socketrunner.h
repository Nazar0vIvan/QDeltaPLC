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

    int socketState() const { return m_socketState; }

    void start();

signals:
    void logMessage(const LoggerMessage& msg);
    void bufferChanged();
    void socketStateChanged();

public slots:
    void stop();

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

    Q_INVOKABLE void connectToHost(const QVariantMap& data);
    Q_INVOKABLE void disconnectFromHost();
    Q_INVOKABLE void writeMessage(const QString& msg);
};

class UdpSocketRunner : public AbstractSocketRunner
{
    Q_OBJECT

public:
    explicit UdpSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);

    Q_PROPERTY(QVariant lastReading READ lastReading NOTIFY lastReadingChanged)
    Q_PROPERTY(QVariant isStreaming READ isStreaming NOTIFY isStreamingChanged)

    Q_INVOKABLE void startStreaming(const QVariantMap& data);
    Q_INVOKABLE void stopStreaming();

    QVariant lastReading() const { return m_lastReading; }
    bool isStreaming() const { return m_isStreaming; }

signals:
    void lastReadingChanged();
    void isStreamingChanged();

public slots:
    void onBufferReady(const QVector<QPointF>& points);

private slots:
    void onPulse();

private:
    QVariant m_lastReading = QVariant{};
    bool m_isStreaming = false;
    QTimer m_timer;
};

#endif // SOCKETRUNNER_H
