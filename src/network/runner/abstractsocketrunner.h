#pragma once

#include <QAbstractSocket>
#include <QHash>
#include <QMetaMethod>
#include <QObject>
#include <QVariantMap>

#include "logger.h"

class QThread;

class AbstractSocketRunner : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)

public:
  explicit AbstractSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
  ~AbstractSocketRunner() override;

  Q_INVOKABLE void invoke(const QString& method, const QVariantMap& args = {});
  Q_PROPERTY(bool isConnected READ isConnected NOTIFY socketStateChanged)
  Q_PROPERTY(bool isDisconnected READ isDisconnected NOTIFY socketStateChanged)

  bool isConnected() const;
  bool isDisconnected() const;
	int socketState() const;

signals:
  void logMessage(const LoggerMessage& msg);
  void bufferChanged();
  void socketStateChanged();

public slots:
  void start();
  void stop();

  void onSocketStateChanged(QAbstractSocket::SocketState state);
  void onThreadStarted();
  void onThreadFinished();

protected:
  QAbstractSocket* m_socket = nullptr;

private:
	void attachSocket(QAbstractSocket* sock);

	QThread* m_thread = nullptr;
	QHash<QString, QMetaMethod> m_api;
	int m_socketState = QAbstractSocket::UnconnectedState;
};
