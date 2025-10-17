#include "socketdeltaplc.h"

QByteArray networkByteOrder(const QByteArray& data) {
  QByteArray result = data;
  for (int i = 0; i + 1 < result.size(); i += 2) {
    std::swap(result[i], result[i + 1]);
  }
  return result;
}

SocketDeltaPLC::SocketDeltaPLC(const QString& name, QObject* parent) : QTcpSocket(parent)
{
  this->setObjectName(name);

  connect(this, &SocketDeltaPLC::errorOccurred, this, &SocketDeltaPLC::onErrorOccurred);
  connect(this, &SocketDeltaPLC::stateChanged,  this, &SocketDeltaPLC::onStateChanged);
  connect(this, &SocketDeltaPLC::connected,     this, &SocketDeltaPLC::onConnected);
  connect(this, &SocketDeltaPLC::readyRead,     this, &SocketDeltaPLC::onReadyRead);

  connect(this, &SocketDeltaPLC::logMessage,  Logger::instance(), &Logger::push);
}

SocketDeltaPLC::~SocketDeltaPLC() {}

// Q_INVOKABLE

void SocketDeltaPLC::connectToHost()
{
  if (m_pa.isNull()) {
    emit logMessage({"Peer address is not set", 0, objectName()});
    return;
  }
  if (!m_pp) {
    emit logMessage({"Peer port is not set", 0, objectName()});
    return;
  }
  QTcpSocket::connectToHost(m_pa, m_pp, QAbstractSocket::ReadWrite);
}

void SocketDeltaPLC::disconnectFromHost()
{
  QTcpSocket::disconnectFromHost();
}

void SocketDeltaPLC::writeMessage(const QVariantMap& cmd)
{
  QByteArray tosend;
  const quint8 id = static_cast<quint8>(cmd.value("id").toString().at(0).toLatin1());
  switch (id) {
    case 89: { // Y
        const quint8 module = static_cast<quint8>(cmd.value("module").toInt());
        const quint8 output = static_cast<quint8>(cmd.value("output").toInt());
        const quint8 state  = cmd.value("state").toBool() ? 1 : 0;

        tosend.reserve(4);
        tosend.append(char(id));
        tosend.append(char(module));
        tosend.append(char(output));
        tosend.append(char(state));

        qDebug() << "bytes hex =" << tosend.toHex(' ') << ", size =" << tosend.size();
        break;
      }
    case 1: { // send raw string
        QByteArray msg = cmd.value("message").toByteArray();
        tosend.append(char(1));
        tosend.append(msg);
        break;
      }
  }

  const qint64 bytesCount = write(tosend);

  if (bytesCount == -1) {
    emit logMessage({"No bytes were written", 0, objectName()});
  }
  emit logMessage({QString::number(bytesCount) + " bytes were written to PLC", 4, objectName()});
}

void SocketDeltaPLC::setSocketConfig(const QVariantMap &config)
{
  m_la = QHostAddress(config.value("localAddress").toString());
  m_lp = config.value("localPort").toUInt();
  m_pa = QHostAddress(config.value("peerAddress").toString());
  m_pp = config.value("peerPort").toUInt();

  if (!bind(m_la,m_lp,QAbstractSocket::ReuseAddressHint)) {
    emit logMessage({"binding failed", 0, objectName()});
    return;
  }

  emit logMessage({stateToString(state()) + "<br/>" +
                  "[local address]: " + m_la.toString() + "<br/>" +
                  "[local port]: " + QString::number(m_lp) + "<br/>" +
                  "[peer address]: " + m_pa.toString() + "<br/>" +
                  "[peer port]: " + QString::number(m_pp) + "<br/>" +
                  "----------",
                  1, objectName()});
}

// PUBLIC SLOTS

void SocketDeltaPLC::onErrorOccurred(QAbstractSocket::SocketError socketError) {
  emit logMessage({this->errorString(), 0, objectName()});
}

void SocketDeltaPLC::onStateChanged(QAbstractSocket::SocketState state) {
  emit logMessage({stateToString(state), 2, objectName()});
}

void SocketDeltaPLC::onConnected()
{
  emit logMessage({"Connection has been successfully established", 1, objectName()});

  QByteArray tosend;
  tosend.append("SYNC");

  const qint64 bytesCount = write(networkByteOrder(tosend));
}

void SocketDeltaPLC::onReadyRead()
{
  QByteArray chunk = readAll();
  const qint64 n = chunk.size();

  QVector<bool> bits(8);
  quint8 firstByte = static_cast<quint8>(chunk[0]);
  for (int i = 0; i < 8; i++) {
    bits[i] = (firstByte >> i) & 1;
  }

  // qDebug() << bits;

  quint8 secondByte = static_cast<quint8>(chunk[1]);
  for (int i = 0; i < 8; i++) {
    bits[i] = (secondByte >> i) & 1;
  }

  // qDebug() << bits;

  emit logMessage({QString::number(n) + " bytes were read from PLC", 3, objectName()});
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




