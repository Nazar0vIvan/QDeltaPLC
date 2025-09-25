#include <QCoreApplication>
#include "socketrunner.h"

// ----- AbstractSocketRunner -----

AbstractSocketRunner::AbstractSocketRunner(QAbstractSocket* socket, QObject* parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_thread->setObjectName(QStringLiteral("SocketRunnerThread"));

    attachSocket(socket);

    connect(this, &AbstractSocketRunner::logMessage,  Logger::instance(), &Logger::push);
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]{ stop(); });
}

AbstractSocketRunner::~AbstractSocketRunner()
{
    stop();
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
}

void AbstractSocketRunner::attachSocket(QAbstractSocket* sock)
{
    Q_ASSERT(sock);
    if (sock->parent()) sock->setParent(nullptr);

    m_socket = sock;

    m_socketState = m_socket->state();
    emit socketStateChanged();

    m_socket->moveToThread(m_thread);

    connect(m_thread, &QThread::started, this, &AbstractSocketRunner::slotThreadStarted);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    connect(m_thread, &QThread::finished, m_socket, &QObject::deleteLater);
    connect(m_socket, &QObject::destroyed, m_thread, &QThread::quit);

    connect(m_socket, &QAbstractSocket::stateChanged,
                     this, [this](QAbstractSocket::SocketState s){
                         if (m_socketState == static_cast<int>(s)) return;
                         m_socketState = static_cast<int>(s);
                         emit socketStateChanged();
                     },
                     Qt::QueuedConnection);
}

void AbstractSocketRunner::start()
{
    if (!m_thread->isRunning())
        m_thread->start();
}

void AbstractSocketRunner::stop()
{
    if (!m_socket) return;
    QMetaObject::invokeMethod(m_socket, "disconnectFromHost", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_socket, "deleteLater", Qt::QueuedConnection);
}

void AbstractSocketRunner::slotThreadStarted()
{
    emit logMessage({"The thread has started", 1, m_socket->objectName()});
}

void AbstractSocketRunner::slotThreadFinished()
{
    emit logMessage({"The thread has finished", 1, m_socket->objectName()});
}

// ----- TcpSocketRunner -----

void TcpSocketRunner::connectToHost(const QVariantMap &data)
{
    QMetaObject::invokeMethod(m_socket, "connectToHost", Qt::QueuedConnection, Q_ARG(QVariantMap, data));
}

void TcpSocketRunner::disconnectFromHost()
{
    QMetaObject::invokeMethod(m_socket, "disconnectFromHost", Qt::QueuedConnection);
}

void TcpSocketRunner::writeMessage(const QString& msg)
{
    QMetaObject::invokeMethod(m_socket, "writeMessage", Qt::QueuedConnection, Q_ARG(QString, msg));
}

// ----- UdpSocketRunner -----

void UdpSocketRunner::startStreaming(const QVariantMap &data)
{
    QMetaObject::invokeMethod(m_socket, "startStreaming", Qt::QueuedConnection, Q_ARG(QVariantMap, data));
}

void UdpSocketRunner::stopStreaming()
{
    QMetaObject::invokeMethod(m_socket, "stopStreaming", Qt::QueuedConnection);
}




