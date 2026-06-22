#include "network/runner/abstractsocketrunner.h"

#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaType>

#include <algorithm>
#include <utility>

AbstractSocketRunner::AbstractSocketRunner(QAbstractSocket* socket, QObject* parent) : QObject(parent)
{
  m_thread = new QThread(this);
  m_thread->setObjectName(QStringLiteral("SocketRunnerThread"));

  attachSocket(socket);
  m_api = invokableMethodNames();

  connect(this, &AbstractSocketRunner::logMessage, Logger::instance(), &Logger::push);
}

AbstractSocketRunner::~AbstractSocketRunner()
{
  // CAN'T just call m_socket destructor because it is in the working thread (managed by m_thread)
  // m_thread itself deletes via parenting to AbstractSocketRunner
  stop(); // this is a fallback in case if runner is destroyed manually (not by app closing)
}


bool AbstractSocketRunner::allowed(const QString& methodName) const
{
  return m_api.contains(methodName);
}

int AbstractSocketRunner::indexOfSignature(const QByteArray& sig) const
{
  if (!m_socket) return -1;
  const QByteArray norm = QMetaObject::normalizedSignature(sig.constData());
  for (auto* mo = m_socket->metaObject(); mo; mo = mo->superClass()) {
    const int idx = mo->indexOfMethod(norm.constData());
    if (idx >= 0) return idx;
  }
  return -1;
}

void AbstractSocketRunner::invoke(const QString& method, const QVariantMap& args)
{
  if (!m_socket) {
    emit logMessage({method + ": socket is null", 0, ""});
    return;
  }

  // Enforce allow-list (your m_api is built from the socket's own public invokables/slots)
  if (!allowed(method)) {
    emit logMessage({method + ": not allowed", 0, m_socket->objectName()});
    return;
  }

  // If somebody calls invoke() before start(), ensure thread is running.
  if (m_thread && !m_thread->isRunning()) {
    m_thread->start();
  }

  const QByteArray name = method.toLatin1();
  const bool haveArgs = !args.isEmpty();

  // Find the method index (signature: () or (QVariantMap))
  int idx = -1;
  if (haveArgs) {
    idx = indexOfSignature(name + "(QVariantMap)");
  } else {
    idx = indexOfSignature(name + "()");
    if (idx < 0) {
       // allow calling (QVariantMap) with empty args if that's the only signature
      idx = indexOfSignature(name + "(QVariantMap)");
    }
  }

  if (idx < 0) {
    emit logMessage({method + ": no such invokable signature", 0, m_socket->objectName()});
    return;
  }

  const QMetaMethod mm = m_socket->metaObject()->method(idx);
  const int retTypeId = mm.returnMetaType().id();
  const bool returnsMap = retTypeId == QMetaType::QVariantMap;
  const bool returnsVoid = retTypeId == QMetaType::Void;

  if (!returnsVoid && !returnsMap) {
    emit logMessage({method + ": unsupported return type (only void/QVariantMap allowed)", 0, m_socket->objectName()});
    return;
  }

  // if already on socket thread, do DirectConnection.
  const bool sameThread = QThread::currentThread() == m_socket->thread();
  const Qt::ConnectionType ct = returnsMap
          ? (sameThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection)
          : (sameThread ? Qt::DirectConnection : Qt::QueuedConnection);

  bool ok = false;
  QVariantMap out;

  if (returnsMap) {
      ok = QMetaObject::invokeMethod(
          m_socket,
          name.constData(),
          ct,
          Q_RETURN_ARG(QVariantMap, out),
          Q_ARG(QVariantMap, args)
      );
    } else {
      if (mm.parameterCount() == 1) {
          ok = QMetaObject::invokeMethod(
              m_socket,
              name.constData(),
              ct,
              Q_ARG(QVariantMap, args)
          );
        } else {
          ok = QMetaObject::invokeMethod(m_socket, name.constData(), ct);
        }
    }

  if (ok)  emit resultReady(method, out);
  else emit logMessage({method + ": invoke failed", 0, m_socket->objectName()});
}

void AbstractSocketRunner::start()
{
  if (m_thread && !m_thread->isRunning()) {
      m_thread->start();
    }
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
  if (m_thread && m_thread->isRunning()) {
    QMetaObject::invokeMethod(sock, [sock] {
      sock->disconnectFromHost();
      sock->close();
      delete sock;
    }, Qt::BlockingQueuedConnection); // blocks Main Thread during lambda execution
    m_thread->quit();
    m_thread->wait();
  } else {
    delete sock;
  }
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
  connect(m_thread, &QThread::finished, this, &AbstractSocketRunner::onThreadFinished);
  connect(m_socket, &QAbstractSocket::stateChanged, this, &AbstractSocketRunner::onSocketStateChanged, Qt::QueuedConnection);
}

QStringList AbstractSocketRunner::invokableMethodNames() const
{
  QStringList out;
  if (!m_socket) return out;

  const QMetaObject* mo = m_socket->metaObject();

  for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
    const QMetaMethod mm = mo->method(i);
    if (mm.methodType() == QMetaMethod::Method && mm.access() == QMetaMethod::Public) {
      out << QString::fromLatin1(mm.name());
    }
  }

  out.removeDuplicates();
  std::sort(out.begin(), out.end());
  return out;
}

// SLOTS

void AbstractSocketRunner::onSocketStateChanged(QAbstractSocket::SocketState state)
{
  const int newState = static_cast<int>(state);
  if (m_socketState == newState) return;
  m_socketState = newState;
  emit socketStateChanged();
}

void AbstractSocketRunner::onThreadStarted()
{
  emit logMessage({"The thread has started", 1, m_socket ? m_socket->objectName() : ""});
}

void AbstractSocketRunner::onThreadFinished()
{
  emit logMessage({"The thread has finished", 1, ""});
}