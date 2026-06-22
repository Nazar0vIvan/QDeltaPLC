#pragma once

#include "network/runner/tcpsocketrunner.h"

#include <QVariantMap>

class QTcpSocket;

class PlcRunner : public TcpSocketRunner
{
  Q_OBJECT

public:
  explicit PlcRunner(QTcpSocket* socket, QObject* parent = nullptr);
  ~PlcRunner() override = default;

signals:
  void dataReady(const QVariantMap& data);
};