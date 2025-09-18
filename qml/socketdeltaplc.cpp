#include "socketdeltaplc.h"

SocketDeltaPLC::SocketDeltaPLC(const QString& name, QObject* parent) : QTcpSocket(parent), hostName{name} {

    connect(this, &SocketDeltaPLC::errorOccurred, this, &SocketDeltaPLC::slotErrorOccurred);
    connect(this, &SocketDeltaPLC::stateChanged, this, &SocketDeltaPLC::slotStateChanged);
    connect(this, &SocketDeltaPLC::connected, this, &SocketDeltaPLC::slotConnectedMessage);
    connect(this, &SocketDeltaPLC::readyRead,  this, &SocketDeltaPLC::slotReadyRead);

    connect(this, &SocketDeltaPLC::logMessage,  Logger::instance(), &Logger::push);
}
SocketDeltaPLC::~SocketDeltaPLC() {}

// PUBLIC SLOTS
void SocketDeltaPLC::connectToHost(const QVariantMap &data)
{
    QHostAddress la = QHostAddress(data.value("localAddress").toString());
    qint16 lp = data.value("localPort").toUInt();
    QHostAddress pa = QHostAddress(data.value("peerAddress").toString());
    qint16 pp = data.value("peerPort").toUInt();

    QTcpSocket::connectToHost(pa, pp, QAbstractSocket::ReadWrite);
}

void SocketDeltaPLC::slotErrorOccurred(QAbstractSocket::SocketError socketError) {
    emit logMessage({this->errorString(), 0, hostName});
}

void SocketDeltaPLC::slotStateChanged(QAbstractSocket::SocketState state) {
    emit logMessage({stateToString(state), 2, hostName});
}

void SocketDeltaPLC::slotConnectedMessage()
{
    emit logMessage({"Connection has been successfully established", 1, hostName});
}

void SocketDeltaPLC::disconnectFromHost()
{
    QTcpSocket::disconnectFromHost();
}

void SocketDeltaPLC::slotReadyRead()
{
    QByteArray chunk = readAll();
    qDebug() << chunk;
    const qint64 n = chunk.size();
    emit logMessage({QString::number(n) + " bytes were read from PLC", 3, hostName});
}

void SocketDeltaPLC::writeMessage(const QString& msg)
{
    const qint64 bytesCount = write(msg.toUtf8());
    if (bytesCount == -1) {
        emit logMessage({"No bytes were written", 0, hostName});
    }
    emit logMessage({QString::number(bytesCount) + " bytes were written to PLC", 4, hostName});
}

// PRIVATE
bool SocketDeltaPLC::tearDownToUnconnected(int ms)
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

QString SocketDeltaPLC::stateToString(SocketState state)
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




