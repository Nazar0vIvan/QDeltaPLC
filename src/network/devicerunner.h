#pragma once

#include <QAbstractSocket>
#include <QObject>
#include <QQmlPropertyMap>
#include <QVariantHash>
#include <QVariantMap>

class AbstractDevice;

class DeviceRunner : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QQmlPropertyMap* data READ data CONSTANT)
  Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)
  Q_PROPERTY(bool isConnected READ isConnected NOTIFY socketStateChanged)
  Q_PROPERTY(bool isDisconnected READ isDisconnected NOTIFY socketStateChanged)

public:
  explicit DeviceRunner(AbstractDevice* dev, QObject* parent = nullptr);
  ~DeviceRunner() override = default;

  QQmlPropertyMap* data() const { return m_data; }
  int socketState() const { return m_sockState; }

  bool isConnected() const;
  bool isDisconnected() const;

  Q_INVOKABLE void invoke(const QString& method, const QVariantMap& args = {});

signals:
  void socketStateChanged();

  void startReq();
  void stopReq();
  void invokeReq(const QString& method, const QVariantMap& args);

public slots:
  void start();
  void stop();

private slots:
  void onStateReady(const QVariantHash& vals);
  void onSockState(QAbstractSocket::SocketState state);

private:
  QQmlPropertyMap* m_data = nullptr;
  int m_sockState = QAbstractSocket::UnconnectedState;
};