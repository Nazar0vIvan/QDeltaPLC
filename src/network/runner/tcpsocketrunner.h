#pragma once

#include "network/runner/abstractsocketrunner.h"

class QTcpSocket;

class TcpSocketRunner : public AbstractSocketRunner
{
  Q_OBJECT

public:
  explicit TcpSocketRunner(QTcpSocket* socket, QObject* parent = nullptr);
  ~TcpSocketRunner() override = default;
};