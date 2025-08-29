#include "deltaplcsocket.h"

DeltaPLCSocket::DeltaPLCSocket(QObject* parent) : QTcpSocket(parent) {
    connect(this, &DeltaPLCSocket::socketDataChanged, this, &DeltaPLCSocket::slotSocketDataChanged);

}

void DeltaPLCSocket::slotSocketDataChanged(const QVariantMap &data)
{
    qDebug() << data.value("localAddress").toString();
    qDebug() << data.value("localPort").toInt();
    qDebug() << data.value("peerAddress").toString();
    qDebug() << data.value("peerPort").toInt();
}


