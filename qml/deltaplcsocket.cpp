#include "deltaplcsocket.h"

DeltaPLCSocket::DeltaPLCSocket(const QString& name, QObject* parent) : QTcpSocket(parent), hostName{name} {

    connect(this, &DeltaPLCSocket::errorOccurred, this, &DeltaPLCSocket::slotErrorOccurred);
    connect(this, &DeltaPLCSocket::stateChanged, this, &DeltaPLCSocket::slotStateChanged);
    connect(this, &DeltaPLCSocket::connected, this, &DeltaPLCSocket::slotConnectedMessage);
    connect(this, &DeltaPLCSocket::readyRead,  this, &DeltaPLCSocket::slotReadyRead);

    connect(this, &DeltaPLCSocket::logMessage,  Logger::instance(), &Logger::push);
}
DeltaPLCSocket::~DeltaPLCSocket() {}

// PUBLIC SLOTS
void DeltaPLCSocket::connectToHost(const QVariantMap &data)
{
    QHostAddress la = QHostAddress(data.value("localAddress").toString());
    qint16 lp = data.value("localPort").toUInt();
    QHostAddress pa = QHostAddress(data.value("peerAddress").toString());
    qint16 pp = data.value("peerPort").toUInt();

    QTcpSocket::connectToHost(pa, pp, QAbstractSocket::ReadWrite);
}

void DeltaPLCSocket::slotErrorOccurred(QAbstractSocket::SocketError socketError) {
    emit logMessage({this->errorString(), 0, hostName});
}

void DeltaPLCSocket::slotStateChanged(QAbstractSocket::SocketState state) {
    emit logMessage({stateToString(state), 2, hostName});
}

void DeltaPLCSocket::slotConnectedMessage()
{
    emit logMessage({"Connection has been successfully established", 1, hostName});
}

void DeltaPLCSocket::disconnectFromHost()
{
    QTcpSocket::disconnectFromHost();
}

void DeltaPLCSocket::slotReadyRead()
{
    while (this->bytesAvailable() > 0) {
        QByteArray chunk = this->read(this->bytesAvailable());
        emit logMessage({"[PLC] Raw:" + QString(chunk), 1, hostName});
        // qDebug() << "[PLC] Raw:" << chunk;
    }
}

void DeltaPLCSocket::writeMessage(const QString& msg)
{
    const qint64 bytesCount = write(msg.toUtf8());
    if (bytesCount == -1) {
        emit logMessage({"No bytes were written", 0, hostName});
    }
    emit logMessage({QString::number(bytesCount) + " bytes were written to PLC", 1, hostName});
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

/*
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
    QTcpSocket::connectToHost(pa, pp, QAbstractSocket::ReadWrite);
}
emit socketBindMessage({text, type, hostName});
*/




