#include "network/runner/plcrunner.h"

#include "network/plc/socketdeltaplc.h"

#include <QTcpSocket>

PlcRunner::PlcRunner(QTcpSocket* socket, QObject* parent) : TcpSocketRunner(socket, parent)
{
  auto* plc = qobject_cast<SocketDeltaPLC*>(socket);
  if (!plc) {
    emit logMessage({"PlcRunner: socket is not SocketDeltaPLC", 0, objectName()});
    return;
  }

  connect(plc, &SocketDeltaPLC::dataReady, this, &PlcRunner::dataReady, Qt::QueuedConnection);
}