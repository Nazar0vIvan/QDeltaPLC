#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

class DeviceProfiles : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QStringList names READ names CONSTANT)

public:
  explicit DeviceProfiles(QObject* parent = nullptr);

  QStringList names() const;

  Q_INVOKABLE QVariantMap device(int index) const;

private:
  struct Device
  {
    QString name;
    QString protocol;
    QString openMode;
    QString localAddress;
    int localPort = -1;
    QString peerAddress;
    int peerPort = -1;
  };

  QVector<Device> m_devices;
};
