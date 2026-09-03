#include "deviceprofiles.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

DeviceProfiles::DeviceProfiles(QObject* parent) : QObject(parent)
{
  QFile file(":/json/db.json");

  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "DeviceProfiles: cannot open db.json";
    return;
  }

  QJsonParseError error;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);

  if (error.error != QJsonParseError::NoError) {
    qWarning() << "DeviceProfiles: invalid db.json:"
               << error.errorString();
    return;
  }

  const QJsonArray devices = document.object().value("devices").toArray();

  m_devices.reserve(devices.size());

  for (const QJsonValue& value : devices) {
    const QJsonObject obj = value.toObject();

    Device device;

    device.name = obj.value("name").toString();
    device.protocol = obj.value("protocol").toString();
    device.openMode = obj.value("openMode").toString();
    device.localAddress = obj.value("localAddress").toString();
    device.localPort = obj.value("localPort").toInt(-1);
    device.peerAddress = obj.value("peerAddress").toString();
    device.peerPort = obj.value("peerPort").toInt(-1);

    m_devices.append(std::move(device));
  }
}

QStringList DeviceProfiles::names() const
{
  QStringList result;
  result.reserve(m_devices.size());

  for (const Device& device : m_devices)
    result.append(device.name);

  return result;
}

QVariantMap DeviceProfiles::device(int index) const
{
  if (index < 0 || index >= m_devices.size())
    return {};

  const Device& device = m_devices.at(index);

  return {
      { "name",         device.name },
      { "protocol",     device.protocol },
      { "openMode",     device.openMode },
      { "localAddress", device.localAddress },
      { "localPort",    device.localPort },
      { "peerAddress",  device.peerAddress },
      { "peerPort",     device.peerPort }
  };
}
