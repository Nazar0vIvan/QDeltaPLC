#pragma once

#include <QAbstractSocket>
#include <QObject>
#include <QString>
#include <QVariantHash>
#include <QVariantMap>
#include <QHash>
#include <QMetaMethod>

#include "logger.h"

class AbstractDevice : public QObject
{
  Q_OBJECT

public:
  explicit AbstractDevice(const QString& name, QObject* parent = nullptr);
  ~AbstractDevice() override = default;

signals:
  void stateReady(const QVariantHash& vals);
  void socketStateReady(QAbstractSocket::SocketState state);
  void logMessage(const LoggerMessage& msg);

public slots:
  void start();
  void stop();
  void invoke(const QString& method, const QVariantMap& args = {});

protected:
  virtual void startDevice() = 0;
  virtual void stopDevice() = 0;

  void attachSocket(QAbstractSocket* sock);

private:
  void buildApi();

  QHash<QString, QMetaMethod> m_api;
  bool m_apiReady = false;

  bool m_started = false;
};
