#pragma once

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

class DeviceProfileModel : public QAbstractTableModel
{
  Q_OBJECT

  Q_PROPERTY(QStringList names READ names CONSTANT)

public:
  enum Column {
    DeviceColumn,
    LocalAddressColumn,
    LocalPortColumn,
    PeerAddressColumn,
    PeerPortColumn,
    ProtocolColumn,
    OpenModeColumn,
    StatusColumn,
    ColumnCount
  };
  Q_ENUM(Column)

  enum Role {
    DriverRole = Qt::UserRole + 1
  };

  explicit DeviceProfileModel(QObject* parent = nullptr);

  Q_INVOKABLE QVariantMap device(int row) const;

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  QStringList names() const;

private:
  struct Device
  {
    QString name;
    QString driver;
    QString localAddress;
    int localPort = -1;
    QString peerAddress;
    int peerPort = -1;
    QString protocol;
    QString openMode;
  };

  void load();

  QVector<Device> m_devices;
};
