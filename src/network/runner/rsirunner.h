#pragma once

#include "network/runner/udpsocketrunner.h"

class QUdpSocket;

class RsiRunner : public UdpSocketRunner
{
  Q_OBJECT
  Q_PROPERTY(bool motionActive READ motionActive NOTIFY motionActiveChanged)

public:
  explicit RsiRunner(QUdpSocket* socket, QObject* parent = nullptr);
  ~RsiRunner() override = default;

  bool motionActive() const { return m_motionActive; }

signals:
  void trajectoryReady();
  void motionStarted();
  void motionFinished();
  void motionActiveChanged();

private slots:
  void onMotionStarted();
  void onMotionFinished();
  void onMotionActiveChanged(bool active);

private:
  bool m_motionActive = false;
};