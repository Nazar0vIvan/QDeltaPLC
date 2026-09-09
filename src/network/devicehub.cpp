#include "devicehub.h"

#include "network/abstractdevice.h"
#include "network/devicerunner.h"

#include <QMetaObject>
#include <utility>

DeviceHub::DeviceHub(QObject* parent) : QObject(parent) {
  m_controlIo.setObjectName(QStringLiteral("ControlIO"));
  m_generalIo.setObjectName(QStringLiteral("GeneralIO"));

  m_controlIo.start();
  m_generalIo.start();
}

DeviceHub::~DeviceHub()
{
  stopAll();
}

void DeviceHub::add(const QString& key, AbstractDevice* dev, DeviceGroup group)
{
  Q_ASSERT(dev);
  Q_ASSERT(!key.isEmpty());
  Q_ASSERT(!m_devs.contains(key));
  Q_ASSERT(!dev->parent());
  Q_ASSERT(dev->thread() == thread());

  QThread* io = group == DeviceGroup::Control ? &m_controlIo : &m_generalIo;

  dev->moveToThread(io);

  QObject::connect(io, &QThread::finished, dev, &QObject::deleteLater);

  auto* runner = new DeviceRunner(dev, this);

  m_devs.insert(key, {dev, runner, io});
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
  {
    if (!m_controlIo.isRunning() && !m_generalIo.isRunning()) return;

    Q_ASSERT(QThread::currentThread() != &m_controlIo);
    Q_ASSERT(QThread::currentThread() != &m_generalIo);

    for (const Entry& entry : std::as_const(m_devs)) {
      if (!entry.io->isRunning()) continue;
      QMetaObject::invokeMethod(entry.dev, &AbstractDevice::stop, Qt::BlockingQueuedConnection);
    }

    m_controlIo.quit();
    m_generalIo.quit();

    m_controlIo.wait();
    m_generalIo.wait();
  }
}
