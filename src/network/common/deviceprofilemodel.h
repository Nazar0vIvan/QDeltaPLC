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

  enum Status {
    Disconnected,
    Connected,
    Bound,
    Streaming
  };
  Q_ENUM(Status)

  enum Role {
    StatusRole = Qt::UserRole + 1
  };

  enum Driver {
    UnknownDriver,
    PlcDriver,
    RsiDriver,
    FtsDriver,
    VfdDriver
  };
  Q_ENUM(Driver)

  explicit DeviceProfileModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  QStringList names() const;

  Q_INVOKABLE QVariantMap device(int row) const;

  void setStatus(int row, Status status);

private:
  struct Device
  {
    QString name;
    QString localAddress;
    int localPort = -1;
    QString peerAddress;
    int peerPort = -1;
    QString protocol;
    QString openMode;
    Driver driver = UnknownDriver;
    Status status = Disconnected;
  };

  void load();

  static QString statusText(Status status);
  static Driver driverFromString(const QString& value);

  QVector<Device> m_devices;
};
