#include "deviceprofilemodel.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

DeviceProfileModel::DeviceProfileModel(QObject* parent) : QAbstractTableModel(parent)
{
  load();
}

int DeviceProfileModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : m_devices.size();
}

int DeviceProfileModel::columnCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : ColumnCount;
}

QVariant DeviceProfileModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid()
      || index.row() < 0
      || index.row() >= m_devices.size()) {
    return {};
  }

  const Device& device = m_devices.at(index.row());

  if (role == StatusRole) return device.status;

  if (role != Qt::DisplayRole) return {};

  switch (index.column()) {
    case DeviceColumn:
      return device.name.isEmpty() ? tr("N/D") : device.name;
    case LocalAddressColumn:
      return device.localAddress.isEmpty()
                 ? tr("N/D")
                 : device.localAddress;
    case LocalPortColumn:
      return device.localPort >= 0
                 ? QVariant(device.localPort)
                 : QVariant(tr("N/D"));
    case PeerAddressColumn:
      return device.peerAddress.isEmpty()
                 ? tr("N/D")
                 : device.peerAddress;
    case PeerPortColumn:
      return device.peerPort >= 0
                 ? QVariant(device.peerPort)
                 : QVariant(tr("N/D"));
    case ProtocolColumn:
      return device.protocol.isEmpty()
                 ? tr("N/D")
                 : device.protocol;
    case OpenModeColumn:
      return device.openMode.isEmpty()
                 ? tr("N/D")
                 : device.openMode;
    case StatusColumn:
      return statusText(device.status);
    default:
      return {};
  }
}

QVariant DeviceProfileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
    return {};
  }

  switch (section) {
    case DeviceColumn: return tr("Device");
    case LocalAddressColumn: return tr("Local Address");
    case LocalPortColumn: return tr("Local Port");
    case PeerAddressColumn: return tr("Peer Address");
    case PeerPortColumn: return tr("Peer Port");
    case ProtocolColumn: return tr("Protocol");
    case OpenModeColumn: return tr("Open Mode");
    case StatusColumn: return tr("Status");
    default: return {};
  }
}

QHash<int, QByteArray> DeviceProfileModel::roleNames() const
{
  auto roles = QAbstractTableModel::roleNames();
  roles.insert(StatusRole, "status");
  return roles;
}

QStringList DeviceProfileModel::names() const
{
  QStringList result;
  result.reserve(m_devices.size());

  for (const Device& device : m_devices)
    result.append(device.name);

  return result;
}

QVariantMap DeviceProfileModel::device(int row) const
{
  if (row < 0 || row >= m_devices.size())
    return {};

  const Device& device = m_devices.at(row);

  return {
    { "name", device.name },
    { "localAddress", device.localAddress },
    { "localPort", device.localPort },
    { "peerAddress", device.peerAddress },
    { "peerPort", device.peerPort },
    { "protocol", device.protocol },
    { "openMode", device.openMode },
    { "driver", static_cast<int>(device.driver) },
    { "status", device.status }
  };
}

void DeviceProfileModel::setStatus(int row, Status status)
{
  if (row < 0 || row >= m_devices.size())
    return;

  Device& device = m_devices[row];

  if (device.status == status) return;

  device.status = status;

  const QModelIndex modelIndex =  index(row, StatusColumn);

  emit dataChanged(modelIndex, modelIndex, { Qt::DisplayRole, StatusRole });
}

void DeviceProfileModel::load()
{
  QFile file(":/json/db.json");

  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "DeviceProfileModel: cannot open db.json";
    return;
  }

  QJsonParseError error;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);

  if (error.error != QJsonParseError::NoError) {
    qWarning()
    << "DeviceProfileModel: invalid db.json:"
    << error.errorString();
    return;
  }

  if (!document.isObject()) {
    qWarning()
    << "DeviceProfileModel: db.json root must be an object";
    return;
  }

  const QJsonArray devices = document.object().value("devices").toArray();

  m_devices.reserve(devices.size());

  for (const QJsonValue& value : devices) {
    if (!value.isObject()) continue;

    const QJsonObject obj = value.toObject();

    Device device;
    device.name = obj.value("name").toString();
    device.localAddress =  obj.value("localAddress").toString();
    device.localPort = obj.value("localPort").toInt(-1);
    device.peerAddress = obj.value("peerAddress").toString();
    device.peerPort = obj.value("peerPort").isNull()
                      ? -1
                      : obj.value("peerPort").toInt(-1);
    device.protocol = obj.value("protocol").toString();
    device.openMode = obj.value("openMode").toString();
    device.driver = driverFromString(obj.value("driver").toString());

    m_devices.append(std::move(device));
  }
}

DeviceProfileModel::Driver
DeviceProfileModel::driverFromString(const QString& value)
{
  const QString driver = value.trimmed().toLower();

  if (driver == "plc")
    return PlcDriver;

  if (driver == "rsi")
    return RsiDriver;

  if (driver == "fts")
    return FtsDriver;

  if (driver == "vfd")
    return VfdDriver;

  return UnknownDriver;
}

QString DeviceProfileModel::statusText(Status status)
{
  switch (status) {
    case Disconnected: return tr("Disconnected");
    case Connected: return tr("Connected");
    case Bound: return tr("Bound");
    case Streaming: return tr("Streaming");
  }

  return {};
}
