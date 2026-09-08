#include "network/devicerunner.h"

#include "network/abstractdevice.h"

#include <QThread>

DeviceRunner::DeviceRunner(AbstractDevice* dev, QObject* parent): QObject(parent),
  m_data(new QQmlPropertyMap(this)) {
  Q_ASSERT(dev);

  QObject::connect(this, &DeviceRunner::startReq, dev, &AbstractDevice::start, Qt::QueuedConnection);
  QObject::connect(this, &DeviceRunner::stopReq, dev, &AbstractDevice::stop, Qt::QueuedConnection);
  QObject::connect(this, &DeviceRunner::invokeReq, dev, &AbstractDevice::invoke, Qt::QueuedConnection);
  QObject::connect(dev, &AbstractDevice::stateReady, this, &DeviceRunner::onStateReady, Qt::QueuedConnection);
  QObject::connect(dev, &AbstractDevice::socketStateReady, this, &DeviceRunner::onSockState, Qt::QueuedConnection);
}

bool DeviceRunner::isConnected() const
{
  return m_sockState == QAbstractSocket::ConnectedState
      || m_sockState == QAbstractSocket::BoundState;
}

bool DeviceRunner::isDisconnected() const
{
  return m_sockState == QAbstractSocket::UnconnectedState;
}

void DeviceRunner::invoke(const QString& method, const QVariantMap& args)
{
  Q_ASSERT(QThread::currentThread() == thread());

  emit invokeReq(method, args);
}

void DeviceRunner::start()
{
  emit startReq();
}

void DeviceRunner::stop()
{
  emit stopReq();
}

void DeviceRunner::onStateReady(const QVariantHash& vals)
{
  if (vals.isEmpty()) return;
  m_data->insert(vals);
}

void DeviceRunner::onSockState(QAbstractSocket::SocketState state)
{
  const int value = static_cast<int>(state);
  if (m_sockState == value) return;

  m_sockState = value;
  emit socketStateChanged();
}
