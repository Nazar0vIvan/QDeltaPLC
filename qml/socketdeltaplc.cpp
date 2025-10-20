#include "socketdeltaplc.h"

namespace {
  constexpr quint16 MAGIC = 0x5AA5;
  constexpr quint8  VER   = 0x01;

  enum : quint8 {
    TYPE_REQ      =0x10,
    TYPE_RESP_OK  =0x11,
    TYPE_RESP_ERR =0x12
  };

  enum : quint8 {
    CMD_READ_IO   = 0x0F,
    CMD_READ_REG  = 0xF0,
    CMD_WRITE_IO  = 0x3C,
    CMD_WRITE_REG = 0xC3,
    CMD_SNAPSHOT  = 0x5A,
    CMD_EXEC      = 0xA5
  };
}


QByteArray swapBytePairs(const QByteArray& data) {
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

void SocketDeltaPLC::writeMessage(const QVariantMap& msg)
{
  QByteArray tosend;
  QByteArray cmdId = msg.value("cmdId").toByteArray();
  if (cmdId == "SET") {
    const quint8 dest = static_cast<quint8>(msg.value("dest").toUInt());
    const quint8 module = static_cast<quint8>(msg.value("module").toUInt());
    const quint8 output = static_cast<quint8>(msg.value("output").toUInt());
    const quint8 state  = msg.value("state").toBool() ? 1 : 0;

    cmdId.resize(4, '\0');
    tosend = cmdId.append(dest).append(module).append(output).append(state);
  }
  else if (cmdId == "SYNC") {
    tosend = cmdId;
  }
  else {
    tosend = msg.value("message").toByteArray();
  }

  const qint64 n = write(swapBytePairs(tosend));

  if (n == -1)
    emit logMessage({"No bytes were written", 0, objectName()});
  else
    emit logMessage({"Message has been wriiten to PLC: " + tosend.toHex(' ') + ", size =" + QString::number(n), 4, objectName()});
}

void SocketDeltaPLC::setSocketConfig(const QVariantMap &config)
{
  m_la = QHostAddress(config.value("localAddress").toString());
  m_lp = config.value("localPort").toUInt();
  m_pa = QHostAddress(config.value("peerAddress").toString());
  m_pp = config.value("peerPort").toUInt();

  emit logMessage({QString("Socket configured:<br/>"
                   "&nbsp;&nbsp;Local: &nbsp;%1:%2<br/>"
                   "&nbsp;&nbsp;Peer: &nbsp;&nbsp;%3:%4<br/>").
                   arg(m_la.toString()).arg(m_lp).arg(m_pa.toString()).arg(m_pp),
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
  const qint64 n = write(swapBytePairs(QByteArray("SYNC")));
  emit logMessage({"Message has been wriiten to PLC: " + tosend.toHex(' ') + ", size =" + QString::number(n), 3, objectName()});
}

void SocketDeltaPLC::onReadyRead()
{
  QByteArray toread = swapBytePairs(readAll());
  QVariantMap plcData;

  char id = toread.at(0);

  qDebug() << "READ" << "bytes hex =" << toread.toHex(' ') << ", size =" << toread.size();

  if (id == 'Y') {
    quint8 moduleIndex = static_cast<quint8>(toread[1]);
    quint8 outputs = static_cast<quint8>(toread[3]);
    QVariantList bits(8);
    for (int i = 0; i < 8; i++) {
        bits[i] = (outputs >> i) & 1;
    }
    plcData = { {"id", "Y"}, {"moduleIndex", moduleIndex}, {"outputs", bits} };
  }

  emit plcDataReady(plcData);
  emit logMessage({QString::number(toread.size()) + " bytes were read from PLC", 3, objectName()});
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




