#include "socketrunner.h"

// ----- AbstractSocketRunner -----

AbstractSocketRunner::AbstractSocketRunner(QAbstractSocket* socket, QObject* parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_thread->setObjectName(QStringLiteral("SocketRunnerThread"));

    attachSocket(socket);

    connect(this, &AbstractSocketRunner::logMessage,  Logger::instance(), &Logger::push);
}

AbstractSocketRunner::~AbstractSocketRunner()
{
    // CAN'T just call m_socket destructor because it is in the working thread (managed by m_thread)
    // m_thread itself deletes via parenting to AbstractSocketRunner
    stop(); // this is a fallback in case if runner is destroyed manually (not by app closing)
}

void AbstractSocketRunner::start()
{
    if (!m_thread->isRunning())
        m_thread->start();
}

void AbstractSocketRunner::stop()
{
    // you can't directly access m_socket, it is in working thread
    // guard, if you accidentally call stop() second time it will return
    auto* sock = std::exchange(m_socket, nullptr);
    if (!sock) return;

    //  The following must be done:
    // - block Main Thread
    // - disconnect, close and delete socket in working thread
    // - finish working thread EL
    QMetaObject::invokeMethod(sock, [sock]{
        sock->disconnectFromHost();
        sock->close();
        delete sock;
    }, Qt::BlockingQueuedConnection); // blocks Main Thread during lambda execution
    m_thread->quit();
    m_thread->wait();
}

void AbstractSocketRunner::attachSocket(QAbstractSocket* sock)
{
    Q_ASSERT(sock);
    if (sock->parent()) sock->setParent(nullptr);

    m_socket = sock;

    m_socketState = m_socket->state();
    emit socketStateChanged();

    m_socket->moveToThread(m_thread); // !!!

    connect(m_thread, &QThread::started, this, &AbstractSocketRunner::onThreadStarted);
    connect(m_socket, &QAbstractSocket::stateChanged, this, &AbstractSocketRunner::onSocketStateChanged, Qt::QueuedConnection);
}

void AbstractSocketRunner::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    if (m_socketState == static_cast<int>(state)) return;
    m_socketState = static_cast<int>(state);
    emit socketStateChanged();
}

void AbstractSocketRunner::onThreadStarted()
{
    emit logMessage({"The thread has started", 1, m_socket->objectName()});
}

void AbstractSocketRunner::onThreadFinished()
{
    emit logMessage({"The thread has finished", 1, m_socket->objectName()});
}

// ----- TcpSocketRunner -----

TcpSocketRunner::TcpSocketRunner(QAbstractSocket* socket, QObject *parent) : AbstractSocketRunner(socket, parent)
{

}

TcpSocketRunner::~TcpSocketRunner() = default;

void TcpSocketRunner::connectToHost(const QVariantMap &data)
{
    if (!m_socket) return;
    QMetaObject::invokeMethod(m_socket, "connectToHost", Qt::QueuedConnection, Q_ARG(QVariantMap, data));
}

void TcpSocketRunner::disconnectFromHost()
{
    QMetaObject::invokeMethod(m_socket, "disconnectFromHost", Qt::QueuedConnection);
}

void TcpSocketRunner::writeMessage(const QVariantMap& cmd)
{
    QMetaObject::invokeMethod(m_socket, "writeMessage", Qt::QueuedConnection, Q_ARG(QVariantMap, cmd));
}

// ----- UdpSocketRunner -----

UdpSocketRunner::UdpSocketRunner(QAbstractSocket *socket, QObject *parent) : AbstractSocketRunner(socket, parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(300);
    connect(&m_timer, &QTimer::timeout, this, [this]{
        if (m_isStreaming) { m_isStreaming = false; emit isStreamingChanged(); }
    });

    connect(m_socket, &QIODevice::readyRead, this, &UdpSocketRunner::onPulse, Qt::QueuedConnection);
    connect(m_socket, &QIODevice::bytesWritten, this, &UdpSocketRunner::onPulse, Qt::QueuedConnection);
}

UdpSocketRunner::~UdpSocketRunner() = default;

void UdpSocketRunner::onPulse()
{
    if (!m_isStreaming) {
        m_isStreaming = true;
        emit isStreamingChanged();
    }
    m_timer.start(); // restart idle countdown on every pulse
}

void UdpSocketRunner::startStreaming(const QVariantMap &data)
{
    QMetaObject::invokeMethod(m_socket, "startStreaming", Qt::QueuedConnection, Q_ARG(QVariantMap, data));
}

void UdpSocketRunner::stopStreaming()
{
    QMetaObject::invokeMethod(m_socket, "stopStreaming", Qt::QueuedConnection);
}

void UdpSocketRunner::onBufferReady(const QVector<QVariantList>& readings)
{
    if(readings.isEmpty()) return;
    m_lastReading = readings.back();
    emit lastReadingChanged();
}




