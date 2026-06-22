#include "network/runner/udpsocketrunner.h"

#include <QUdpSocket>

UdpSocketRunner::UdpSocketRunner(QUdpSocket* socket, QObject* parent) : AbstractSocketRunner(socket, parent)
{
}