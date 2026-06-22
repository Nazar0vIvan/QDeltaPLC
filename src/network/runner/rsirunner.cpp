#include "network/runner/rsirunner.h"

#include "network/rsi/socketrsi.h"

#include <QUdpSocket>

RsiRunner::RsiRunner(QUdpSocket* socket, QObject* parent) : UdpSocketRunner(socket, parent)
{
  auto* rsi = qobject_cast<SocketRSI*>(socket);
  if (!rsi) {
    emit logMessage({"RsiRunner: socket is not SocketRSI", 0, objectName()});
    return;
  }

  connect(rsi, &SocketRSI::motionStarted, this, &RsiRunner::onMotionStarted, Qt::QueuedConnection);
  connect(rsi, &SocketRSI::motionFinished, this, &RsiRunner::onMotionFinished, Qt::QueuedConnection);
  connect(rsi, &SocketRSI::motionActiveChanged, this, &RsiRunner::onMotionActiveChanged, Qt::QueuedConnection);
  connect(rsi, &SocketRSI::trajectoryReady, this, &RsiRunner::trajectoryReady, Qt::QueuedConnection);
}

void RsiRunner::onMotionStarted()
{
  if (!m_motionActive) {
    m_motionActive = true;
    emit motionActiveChanged();
  }
  emit motionStarted();
}

void RsiRunner::onMotionFinished()
{
  if (m_motionActive) {
    m_motionActive = false;
    emit motionActiveChanged();
  }

  emit motionFinished();
}

void RsiRunner::onMotionActiveChanged(bool active)
{
  if (m_motionActive == active) return;

  m_motionActive = active;
  emit motionActiveChanged();
}