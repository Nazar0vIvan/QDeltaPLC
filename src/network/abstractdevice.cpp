#include "network/abstractdevice.h"

#include "network/common/socketstateutils.h"

#include <QMetaMethod>
#include <QThread>

AbstractDevice::AbstractDevice(const QString& name, QObject* parent)
  : QObject(parent) {
  setObjectName(name);

  QObject::connect(
      this,
      &AbstractDevice::logMessage,
      Logger::instance(),
      &Logger::push,
      Qt::QueuedConnection);
}

void AbstractDevice::start()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_started) return;

  if (!m_apiReady) buildApi();

  startDevice();
  m_started = true;
}

void AbstractDevice::stop()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!m_started) return;

  stopDevice();
  m_started = false;
}

void AbstractDevice::invoke(const QString& method, const QVariantMap& args)
{
  Q_ASSERT(QThread::currentThread() == thread());

  if (!m_started) {
    emit logMessage({method + ": device is not started", 0, objectName()});
    return;
  }

  const auto it = m_api.constFind(method);

  if (it == m_api.cend()) {
    emit logMessage({
        QStringLiteral("invoke(\"%1\") is not exposed").arg(method),
        0,
        objectName()
    });
    return;
  }

  const QMetaMethod& mm = it.value();

  const bool ok = mm.parameterCount() == 1
      ? mm.invoke(this, Qt::DirectConnection, Q_ARG(QVariantMap, args))
      : mm.invoke(this, Qt::DirectConnection);

  if (!ok) emit logMessage({method + ": invoke failed", 0, objectName()});
}

void AbstractDevice::buildApi()
{
  const QMetaObject* mo = metaObject();
  const int first = AbstractDevice::staticMetaObject.methodCount();

  for (int i = first; i < mo->methodCount(); ++i) {
    const QMetaMethod mm = mo->method(i);

    if (mm.methodType() != QMetaMethod::Method) continue;
    if (mm.access() != QMetaMethod::Public) continue;
    if (mm.returnMetaType().id() != QMetaType::Void) continue;
    if (mm.parameterCount() > 1) continue;

    if (mm.parameterCount() == 1
        && mm.parameterMetaType(0) != QMetaType::fromType<QVariantMap>()) {
      continue;
    }

    const QString name = QString::fromLatin1(mm.name());

    Q_ASSERT(!m_api.contains(name));
    m_api.insert(name, mm);
  }

  m_apiReady = true;
}

void AbstractDevice::attachSocket(QAbstractSocket* sock)
{
  Q_ASSERT(sock);
  Q_ASSERT(sock->thread() == thread());

  QObject::connect(
      sock,
      &QAbstractSocket::stateChanged,
      this,
      [this](QAbstractSocket::SocketState state) {
    emit socketStateReady(state);
    emit logMessage({socketStateName(state), 2, objectName()});
  });

  QObject::connect(
      sock,
      &QAbstractSocket::errorOccurred,
      this,
      [this, sock](QAbstractSocket::SocketError error) {
    emit logMessage({
      QString("%1: %2").arg(socketErrorName(error), sock->errorString()),
      0,
      objectName()
    });
  });

  emit socketStateReady(sock->state());
}
