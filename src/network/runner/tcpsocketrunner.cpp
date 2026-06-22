#include "network/runner/tcpsocketrunner.h"

#include <QTcpSocket>

TcpSocketRunner::TcpSocketRunner(QTcpSocket* socket, QObject* parent) : AbstractSocketRunner(socket, parent)
{
}