#include "devicehub.h"

#include "network/abstractdevice.h"
#include "network/devicerunner.h"

#include <QMetaObject>
#include <utility>

DeviceHub::DeviceHub(QObject* parent)
  : QObject(parent) {
  m_io.setObjectName(QStringLiteral("DeviceIO"));
  m_io.start();
}

DeviceHub::~DeviceHub()
{
  stopAll();
}

void DeviceHub::add(const QString& key, AbstractDevice* dev)
{
  Q_ASSERT(dev);
  Q_ASSERT(!key.isEmpty());
  Q_ASSERT(!m_devs.contains(key));
  Q_ASSERT(!dev->parent());
  Q_ASSERT(dev->thread() == thread());

  dev->moveToThread(&m_io);

  QObject::connect(
      &m_io,
      &QThread::finished,
      dev,
      &QObject::deleteLater);

  auto* runner = new DeviceRunner(dev, this);

  m_devs.insert(key, {dev, runner});
}

DeviceRunner* DeviceHub::device(const QString& key) const
{
  const auto it = m_devs.constFind(key);
  if (it == m_devs.cend()) return nullptr;

  return it->runner;
}

QStringList DeviceHub::keys() const
{
  return m_devs.keys();
}

void DeviceHub::startAll()
{
  for (const Entry& entry : std::as_const(m_devs))
    entry.runner->start();
}

void DeviceHub::stopAll()
{
  if (!m_io.isRunning()) return;

  Q_ASSERT(QThread::currentThread() != &m_io);

  for (const Entry& entry : std::as_const(m_devs)) {
    QMetaObject::invokeMethod(
        entry.dev,
        &AbstractDevice::stop,
        Qt::BlockingQueuedConnection);
  }

  m_io.quit();
  m_io.wait();
}
