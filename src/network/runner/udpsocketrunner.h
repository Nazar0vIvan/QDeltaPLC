#pragma once

#include "network/runner/abstractsocketrunner.h"

#include <QTimer>

class QUdpSocket;

class UdpSocketRunner : public AbstractSocketRunner
{
  Q_OBJECT
  Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY isStreamingChanged)

public:
  explicit UdpSocketRunner(QUdpSocket* socket, QObject* parent = nullptr);
  ~UdpSocketRunner() override = default;

  bool isStreaming() const { return m_isStreaming; }

signals:
  void isStreamingChanged();

private slots:
  void onPulse();

private:
  bool m_isStreaming = false;
  QTimer m_timer;
};
