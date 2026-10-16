#include "devices/abstractdevice.h"

#include <functional>

#include <QDebug>
#include <QMetaMethod>
#include <QMetaEnum>
#include <QThread>

namespace {

// Socket metadata and the numeric value define the name, including unknown values.
QString socketEnumName(const QMetaEnum& enumeration, int value)
{
  const char* key = enumeration.valueToKey(value);
  return key ? QString::fromLatin1(key) : QStringLiteral("Unknown(%1)").arg(value);
}

} // namespace

AbstractDevice::AbstractDevice(const QString& name, QObject* parent)
  : QObject(parent) {
  setObjectName(name);
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
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << method + ": device is not started";
    return;
  }

  const auto it = m_api.constFind(method);

  if (it == m_api.cend()) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":"
        << QStringLiteral("invoke(\"%1\") is not exposed").arg(method);
    return;
  }

  const QMetaMethod& mm = it.value();

  const bool ok = mm.parameterCount() == 1
      ? mm.invoke(this, Qt::DirectConnection, Q_ARG(QVariantMap, args))
      : mm.invoke(this, Qt::DirectConnection);

  if (!ok) qInfo().noquote() << "[ERROR]" << objectName() + ":" << method + ": invoke failed";
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
      &AbstractDevice::onSocketStateChanged);

  QObject::connect(
      sock,
      &QAbstractSocket::errorOccurred,
      this,
      std::bind(&AbstractDevice::logSocketError, this, sock, std::placeholders::_1));

  emit socketStateReady(sock->state());
}

void AbstractDevice::onSocketStateChanged(QAbstractSocket::SocketState state)
{
  emit socketStateReady(state);
  qInfo().noquote() << "[INFO]" << objectName() + ":" << socketEnumName(
      QMetaEnum::fromType<QAbstractSocket::SocketState>(), static_cast<int>(state));
}

void AbstractDevice::logSocketError(QAbstractSocket* socket, QAbstractSocket::SocketError error)
{
  qInfo().noquote() << "[ERROR]" << objectName() + ":"
      << QString("%1: %2").arg(
          socketEnumName(QMetaEnum::fromType<QAbstractSocket::SocketError>(), static_cast<int>(error)),
          socket->errorString());
}
