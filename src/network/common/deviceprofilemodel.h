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

  explicit DeviceProfileModel(QObject* parent = nullptr);

  Q_INVOKABLE QVariantMap device(int row) const;

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  QStringList names() const;

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
    Status status = Disconnected;
  };

  void load();

  static QString statusText(Status status);

  QVector<Device> m_devices;
};
