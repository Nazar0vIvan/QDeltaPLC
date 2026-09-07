#include "devicehub.h"

#include "network/runner/abstractsocketrunner.h"

#include <utility>

DeviceHub::DeviceHub(QObject* parent) : QObject(parent) {}

void DeviceHub::add(const QString& key, AbstractSocketRunner* runner)
{
  Q_ASSERT(runner);
  Q_ASSERT(!m_devs.contains(key));

  runner->setParent(this);
  m_devs.insert(key, runner);
}

AbstractSocketRunner* DeviceHub::device(const QString& key) const
{
  return m_devs.value(key, nullptr);
}

QStringList DeviceHub::keys() const
{
  return m_devs.keys();
}

void DeviceHub::startAll()
{
  for (AbstractSocketRunner* dev : std::as_const(m_devs)) dev->start();
}

void DeviceHub::stopAll()
{
  for (AbstractSocketRunner* dev : std::as_const(m_devs)) dev->stop();
}
