#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>

class AbstractDevice;
class DeviceRunner;

// The only object QML needs in order to reach a device. QML asks for a runner
// by key; adding a device requires no new QML registration and no new context
// property.
//
// DeviceHub is the parent of every DeviceRunner it holds. AbstractDevice
// objects live in the shared I/O thread and are deleted in that thread.
class DeviceHub : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList keys READ keys CONSTANT)

public:
  explicit DeviceHub(QObject* parent = nullptr);

  ~DeviceHub() override;

  void add(const QString& key, AbstractDevice* dev);

  Q_INVOKABLE DeviceRunner* device(const QString& key) const;

  QStringList keys() const;

public slots:
  void startAll();
  void stopAll();

private:
  struct Entry {
    AbstractDevice* dev = nullptr;
    DeviceRunner* runner = nullptr;
  };

  QThread m_io;
  QHash<QString, Entry> m_devs;
};
