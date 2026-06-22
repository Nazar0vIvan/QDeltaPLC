#pragma once

#include "network/runner/abstractsocketrunner.h"

class QUdpSocket;

class UdpSocketRunner : public AbstractSocketRunner
{
  Q_OBJECT

public:
  explicit UdpSocketRunner(QUdpSocket* socket, QObject* parent = nullptr);
  ~UdpSocketRunner() override = default;
};
