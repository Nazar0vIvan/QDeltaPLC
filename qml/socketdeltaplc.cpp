#include "socketdeltaplc.h"

namespace {
  constexpr quint16 MAGIC = 0x5AA5;
  constexpr quint8  VER   = 0x01;

  enum Type : quint8 {
    REQ      = 0x10,
    RESP_OK  = 0x11,
    RESP_ERR = 0x12
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
}

void SocketDeltaPLC::onReadyRead()
{
  m_rx.append(readAll());
  QVariantMap out; // to qml

  while (true) {
    const ParseResult r = parseNext(m_rx);

    if (r.kind == ParseKind::NeedMore) break;

    if (r.kind == ParseKind::Drop) {
      emit logMessage({"MAGIC/HEADER mismatch: dropping 1 byte to resync", 0, objectName()});
      m_rx.remove(0, 1);
      continue;
    }

    // Good message → consume and dispatch
    const Message& msg = r.msg;
    m_rx.remove(0, msg.consumed);

    out["tid"] = int(msg.header.tid);

    // TID correlation:
    // If there was a request with matching tid ➝ add req to out
    const bool known = m_pending.contains(msg.header.tid);
    out["correlated"] = known;
    if (known) {
      out["req"] = m_pending.value(msg.header.tid).req;
      m_pending.remove(msg.header.tid);
    }

    // Dispatch by TYPE
    if (msg.header.type == Type::RESP_OK) {
      if (msg.payload.size() < 2) {
        emit logMessage({"RESP_OK too short", 0, objectName()});
        continue;
      }
      const quint8 cmd   = quint8(msg.payload[0]);
      const quint8 status  = quint8(msg.payload[1]);
      const QByteArray body = msg.payload.mid(2);

      out["type"]   = "RESP_OK";
      out["cmd"]    = cmd;
      out["status"] = status;

      QVariantMap parsed = parseRespOk(cmd, body);
      for (auto it = parsed.begin(); it != parsed.end(); ++it)
        out[it.key()] = it.value();

      emit plcDataReady(out);
    }
    else if (msg.header.type == Type::RESP_ERR) {
      out["type"] = "RESP_ERR";
      QVariantMap parsed = parseRespErr(msg.payload);
      for (auto it = parsed.begin(); it != parsed.end(); ++it) out[it.key()] = it.value();

      emit plcDataReady(out);
    }
    else {
      emit logMessage({"Unexpected TYPE " + QString::number(msg.header.type), 0, objectName()});
    }
  }
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

ParseResult SocketDeltaPLC::parseNext(const QByteArray &buffer)
{

}




