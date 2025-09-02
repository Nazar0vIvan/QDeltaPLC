#include "deltaplcsocket.h"

DeltaPLCSocket::DeltaPLCSocket(const QString& name, QObject* parent) : QTcpSocket(parent), hostName{name} {

    connect(this, &DeltaPLCSocket::errorOccurred, [&](){ emit errorOccurredMessage({this->errorString(), 0, hostName}); });
    connect(this, &DeltaPLCSocket::socketConfigChanged, Logger::instance(), &Logger::push);
}

DeltaPLCSocket::~DeltaPLCSocket() {}

void DeltaPLCSocket::slotSocketDataChanged(const QVariantMap &data)
{
    auto lp = data.value("localPort").toUInt();
    auto la = QHostAddress(data.value("localAddress").toString());

    QString text; int type = 0;
    if(!tearDownToUnconnected()) {
        text = tr("Failed to return to UnconnectedState: %1").arg(errorString());
    }
    if(!bind(la, lp, QAbstractSocket::DontShareAddress | QAbstractSocket::ReuseAddressHint)) {
        text = tr("bind(%1:%2) failed").arg(localAddress().toString()).arg(localPort());
    } else {
        text = tr("Bind %1:%2").arg(localAddress().toString()).arg(localPort());
        type = 1;
    }
    emit socketConfigChanged({text, type, hostName});
}

bool DeltaPLCSocket::tearDownToUnconnected(int ms)
{
    if (state() == QAbstractSocket::UnconnectedState)
        return true;

    disconnectFromHost();
    if (state() == QAbstractSocket::UnconnectedState)
        return true;

    if (state() == QAbstractSocket::ClosingState) {
        if (waitForDisconnected(ms))
            return true;
    }

    abort();
    close();
    return state() == QAbstractSocket::UnconnectedState;
}


