#pragma once

#include <QAbstractSocket>
#include <QObject>
#include <QStringList>
#include <QThread>
#include <QVariantMap>

#include "logger.h"

class AbstractSocketRunner : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)

public:
  explicit AbstractSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
  ~AbstractSocketRunner() override;

  Q_INVOKABLE void invoke(const QString& method, const QVariantMap& args = {});

  int socketState() const { return m_socketState; }

signals:
  void logMessage(const LoggerMessage& msg);
  void bufferChanged();
  void socketStateChanged();
  void resultReady(const QString& method, const QVariantMap& result);

public slots:
  void start();
  void stop();

  void onSocketStateChanged(QAbstractSocket::SocketState state);
  void onThreadStarted();
  void onThreadFinished();

protected:
  QAbstractSocket* m_socket = nullptr;
  QThread* m_thread = nullptr;
  QStringList m_api = {};

private:
  void attachSocket(QAbstractSocket* sock);
  QStringList invokableMethodNames() const;
  bool allowed(const QString& methodName) const;
  int indexOfSignature(const QByteArray& sig) const;

  int m_socketState = QAbstractSocket::UnconnectedState;
};