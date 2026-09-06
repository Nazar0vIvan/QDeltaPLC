#include "network/runner/abstractsocketrunner.h"

#include <QMetaType>
#include <QStringList>
#include <QThread>

#include <algorithm>
#include <utility>

namespace {

QHash<QString, QMetaMethod> ownInvokables(const QObject* sock)
{
	QHash<QString, QMetaMethod> api;
	const QMetaObject* mo = sock->metaObject();

	for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
		const QMetaMethod mm = mo->method(i);

		if (mm.methodType() != QMetaMethod::Method) continue;
		if (mm.access() != QMetaMethod::Public) continue;

		const int retId = mm.returnMetaType().id();
		if (retId != QMetaType::Void && retId != QMetaType::QVariantMap) continue;

		if (mm.parameterCount() > 1) continue;
		if (mm.parameterCount() == 1
				&& mm.parameterMetaType(0) != QMetaType::fromType<QVariantMap>()) continue;

		api.insert(QString::fromLatin1(mm.name()), mm);
	}

	return api;
}

QString apiList(const QHash<QString, QMetaMethod>& api)
{
	QStringList names = api.keys();
	std::sort(names.begin(), names.end());
	return names.join(QStringLiteral(", "));
}

} // namespace

AbstractSocketRunner::AbstractSocketRunner(QAbstractSocket* socket, QObject* parent) : QObject(parent)
{
  m_thread = new QThread(this);
  m_thread->setObjectName(QStringLiteral("SocketRunnerThread"));

  attachSocket(socket);

  connect(this, &AbstractSocketRunner::logMessage, Logger::instance(), &Logger::push);
}

AbstractSocketRunner::~AbstractSocketRunner()
{
  // CAN'T just call m_socket destructor because it is in the working thread (managed by m_thread)
  // m_thread itself deletes via parenting to AbstractSocketRunner
  stop(); // this is a fallback in case if runner is destroyed manually (not by app closing)
}

// PUBLIC

void AbstractSocketRunner::invoke(const QString& method, const QVariantMap& args)
{
  if (!m_socket) {
    emit logMessage({method + ": socket is null", 0, ""});
    return;
  }

	// Enforce allow-list (your m_api is built from the socket's own public invokables/slots)
	const auto it = m_api.constFind(method);
	if (it == m_api.cend()) {
		emit logMessage({
			QStringLiteral("invoke(\"%1\") is not exposed by %2. Exposed: %3")
			.arg(method,
					 QString::fromLatin1(m_socket->metaObject()->className()),
					 apiList(m_api)),
					 0,
					m_socket->objectName()
		});
		return;
	}


  // If somebody calls invoke() before start(), ensure thread is running.
	if (m_thread && !m_thread->isRunning()) m_thread->start();

	const bool returnsMap = it->returnMetaType().id() == QMetaType::QVariantMap;
	const bool sameThread = QThread::currentThread() == m_socket->thread();

	const Qt::ConnectionType ct = returnsMap
		? (sameThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection)
		: (sameThread ? Qt::DirectConnection : Qt::QueuedConnection);

	bool ok = false;
	QVariantMap out;

	if (returnsMap) {
		ok = it->invoke(m_socket, ct, Q_RETURN_ARG(QVariantMap, out), Q_ARG(QVariantMap, args));
	} else if (it->parameterCount() == 1) {
		ok = it->invoke(m_socket, ct, Q_ARG(QVariantMap, args));
	} else {
		ok = it->invoke(m_socket, ct);
	}

	if (ok) emit resultReady(method, out);
	else emit logMessage({method + ": invoke failed", 0, m_socket->objectName()});
}

bool AbstractSocketRunner::isConnected() const
{
	return m_socketState == QAbstractSocket::ConnectedState
				 || m_socketState == QAbstractSocket::BoundState;
}

bool AbstractSocketRunner::isDisconnected() const
{
	return m_socketState == QAbstractSocket::UnconnectedState;
}

int AbstractSocketRunner::socketState() const
{
	return m_socketState;
}

// SLOTS

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

// PRIVATE

void AbstractSocketRunner::attachSocket(QAbstractSocket* sock)
{
  Q_ASSERT(sock);

  if (sock->parent()) sock->setParent(nullptr);

  m_socket = sock;
	m_api = ownInvokables(sock);

  m_socketState = m_socket->state();
  emit socketStateChanged();

  m_socket->moveToThread(m_thread); // !!!

  connect(m_thread, &QThread::started, this, &AbstractSocketRunner::onThreadStarted);
  connect(m_thread, &QThread::finished, this, &AbstractSocketRunner::onThreadFinished);
  connect(m_socket, &QAbstractSocket::stateChanged, this, &AbstractSocketRunner::onSocketStateChanged, Qt::QueuedConnection);
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
