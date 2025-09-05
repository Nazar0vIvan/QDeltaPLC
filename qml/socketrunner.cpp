#include <QCoreApplication>

#include "socketrunner.h"

static void wire_lifetimes(QThread* thread, QTcpSocket* socket)
{
    QObject::connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    QObject::connect(socket, &QObject::destroyed, thread, &QThread::quit);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

SocketRunner::SocketRunner(QTcpSocket* existingSocket, QObject* parent)
    : QObject(parent)
{
    m_thread = new QThread(this);
    m_thread->setObjectName(QStringLiteral("SocketRunnerThread"));

    attachSocket(existingSocket);

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [this]{ stop(); });
}

SocketRunner::~SocketRunner()
{
    stop();
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
}

void SocketRunner::setConfig(const QVariantMap &data)
{
    QMetaObject::invokeMethod(m_socket, "setConfig", Qt::QueuedConnection, Q_ARG(QVariantMap, data));
}

void SocketRunner::connectToHost()
{
    QMetaObject::invokeMethod(m_socket, "connectToHost", Qt::QueuedConnection);
}

void SocketRunner::disconnectFromHost()
{
    QMetaObject::invokeMethod(m_socket, "disconnectFromHost", Qt::QueuedConnection);
}

void SocketRunner::writeMessage(const QVariantMap &msg)
{

}

void SocketRunner::attachSocket(QTcpSocket* sock)
{
    Q_ASSERT(sock);
    if (sock->parent())
        sock->setParent(nullptr);

    m_socket = sock;
    m_socket->moveToThread(m_thread);
    wire_lifetimes(m_thread, m_socket);

    QObject::connect(m_thread, &QThread::started, m_socket, []{

    });
}

void SocketRunner::start()
{
    if (!m_thread->isRunning())
        m_thread->start();
}

void SocketRunner::stop(bool deleteSocket)
{
    if (!m_socket) return;

    QMetaObject::invokeMethod(m_socket, "disconnectFromHost", Qt::QueuedConnection);
    if (deleteSocket) {
        QMetaObject::invokeMethod(m_socket, "deleteLater", Qt::QueuedConnection);
    }
}
