#include "udpsocketrunner.h"

#include <QIODevice>
#include <QUdpSocket>

UdpSocketRunner::UdpSocketRunner(QUdpSocket* socket, QObject* parent) : AbstractSocketRunner(socket, parent)
{
  m_timer.setSingleShot(true);
  m_timer.setInterval(300);

  connect(&m_timer, &QTimer::timeout, this, [this] {
    if (!m_isStreaming) return;
    m_isStreaming = false;
    emit isStreamingChanged();
  });

  connect(m_socket, &QIODevice::readyRead, this, &UdpSocketRunner::onPulse, Qt::QueuedConnection);
}

void UdpSocketRunner::onPulse()
{
  if (!m_isStreaming) {
    m_isStreaming = true;
    emit isStreamingChanged();
  }

  m_timer.start();
}
