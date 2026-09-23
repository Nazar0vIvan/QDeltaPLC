#pragma once

#include "network/abstractdevice.h"
#include "plcmessagemanager.h"

#include <QByteArray>
#include <QHostAddress>
#include <QHash>
#include <optional>

class QTcpSocket;

class PlcDevice : public AbstractDevice
{
  Q_OBJECT

public:
  explicit PlcDevice(const QString& name, QObject* parent = nullptr);
  ~PlcDevice() override = default;

  Q_INVOKABLE void connect(const QVariantMap& config);
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void writeMessage(const QVariantMap& msg);

protected:
  void startDevice() override;
  void stopDevice() override;

private slots:
  void onConnected();
  void onReadyRead();
  void onStateChanged(QAbstractSocket::SocketState state);

private:
  friend class PlcProtocolTests;
  static QByteArray swapBytes(const QByteArray& data);
  std::optional<quint8> availableTid() const;
  bool matchResponse(const QVariantMap& data);
  void processIncoming(const QByteArray& bytes);

  void publishData(const QVariantMap& data);

  QTcpSocket* m_sock = nullptr;
  PlcMessageManager* m_mgr = nullptr;

  QHostAddress m_pa;
  quint16 m_pp = 0;

  QByteArray m_rx;
  QHash<quint8, QVariantMap> m_pend;
  quint8 m_nextTid = 1;
};
