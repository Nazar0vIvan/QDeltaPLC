#include "deltaplcsocket.h"

DeltaPLCSocket::DeltaPLCSocket(const QString& name, QObject* parent) : QTcpSocket(parent), hostName{name} {

    connect(this, &DeltaPLCSocket::errorOccurred, this, [this](){ emit errorOccurredMessage({this->errorString(), 0, hostName}); });
    connect(this, &DeltaPLCSocket::stateChanged, this, &DeltaPLCSocket::slotStateChanged);

    connect(this, &DeltaPLCSocket::socketBindMessage,  Logger::instance(), &Logger::push);
    connect(this, &DeltaPLCSocket::errorOccurredMessage, Logger::instance(), &Logger::push);
    connect(this, &DeltaPLCSocket::stateChangedMessage,  Logger::instance(), &Logger::push);
}

DeltaPLCSocket::~DeltaPLCSocket() {}

// PUBLIC SLOTS
void DeltaPLCSocket::setConfig(const QVariantMap &data)
{
    QHostAddress la = QHostAddress(data.value("localAddress").toString());
    qint16 lp = data.value("localPort").toUInt();
    QHostAddress pa = QHostAddress(data.value("peerAddress").toString());
    qint16 pp = data.value("peerPort").toUInt();

    QString text;
    int type = 0;

    if(!tearDownToUnconnected()) {
        text = tr("Failed to return to UnconnectedState: %1").arg(errorString());
        emit socketBindMessage({text, type, hostName});
        return;
    }
    if(!bind(la, lp, QAbstractSocket::DontShareAddress | QAbstractSocket::ReuseAddressHint)) {
        text = tr("bind(%1:%2) failed").arg(la.toString()).arg(lp);
    } else {
        text = tr("bind %1:%2").arg(la.toString()).arg(lp);
        type = 1;
        setLocalAddress(la); setLocalPort(lp); setPeerAddress(pa); setPeerPort(pp);
    }
    emit socketBindMessage({text, type, hostName});
}

void DeltaPLCSocket::slotStateChanged(SocketState state)
{
    emit stateChangedMessage({stateToString(state), 2, hostName});
}

void DeltaPLCSocket::connectToHost(const QString &hostName, quint16 port, OpenMode openMode, NetworkLayerProtocol protocol)
{
    QTcpSocket::connectToHost(peerAddress(), peerPort(), QAbstractSocket::ReadWrite);
}

void DeltaPLCSocket::disconnectFromHost()
{
    QTcpSocket::disconnectFromHost();
}

void DeltaPLCSocket::slotReadyRead()
{

}

void DeltaPLCSocket::writeMessage(const QVariantMap &msg)
{

}

// PRIVATE
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
    abort(); close();
    return state() == QAbstractSocket::UnconnectedState;
}

QString DeltaPLCSocket::stateToString(SocketState state)
{
    switch (state) {
        case QAbstractSocket::UnconnectedState: return "UnconnectedState";
        case QAbstractSocket::HostLookupState:  return "HostLookupState";
        case QAbstractSocket::ConnectingState:  return "ConnectingState";
        case QAbstractSocket::ConnectedState:   return "ConnectedState";
        case QAbstractSocket::BoundState:       return "BoundState";
        case QAbstractSocket::ClosingState:     return "ClosingState";
        case QAbstractSocket::ListeningState:   return "ListeningState";
        default: return "UnconnectedState";
    }
}




