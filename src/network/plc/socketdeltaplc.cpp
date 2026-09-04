#include "socketdeltaplc.h"

SocketDeltaPLC::SocketDeltaPLC(const QString& name, QObject* parent) : QTcpSocket(parent)
{
  this->setObjectName(name);

  connect(this, &SocketDeltaPLC::errorOccurred, this, &SocketDeltaPLC::onErrorOccurred);
  connect(this, &SocketDeltaPLC::stateChanged,  this, &SocketDeltaPLC::onStateChanged);
  connect(this, &SocketDeltaPLC::connected,     this, &SocketDeltaPLC::onConnected);
  connect(this, &SocketDeltaPLC::readyRead,     this, &SocketDeltaPLC::onReadyRead);

  connect(this, &SocketDeltaPLC::logMessage, Logger::instance(), &Logger::push);
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
  const PlcMessageManager::ParseResult buildReqResult = m_mgr.buildReq(msg, ++m_nextTid);

  if (!buildReqResult.ok()) {
    emit logMessage({ "WRITE ERROR: " + QString::number(buildReqResult.error), 0, objectName() });
    return;
  }

  QByteArray tosend = buildReqResult.data.toByteArray();
  const qint64 n = write(swapBytes(tosend));

  qDebug() << "WRITE: " << tosend.toHex(' ').toUpper();

  emit logMessage({ (n == -1 ? "No bytes were written" :
                    "TX: " + tosend.toHex(' ').toUpper() + " (" + QString::number(n) + " bytes)"),
                    (n == -1 ? 0 : 4), objectName()});
}

void SocketDeltaPLC::connectDevice(const QVariantMap &config)
{
  const QHostAddress localAddress(config.value("localAddress").toString());
  const QHostAddress peerAddress(config.value("peerAddress").toString());
  bool localPortOk = false;
  bool peerPortOk = false;

  const uint localPort = config.value("localPort").toUInt(&localPortOk);

  const uint peerPort = config.value("peerPort").toUInt(&peerPortOk);

  if (localAddress.isNull()
      || peerAddress.isNull()
      || !localPortOk
      || !peerPortOk
      || localPort > 65535
      || peerPort == 0
      || peerPort > 65535) {

    emit logMessage({
      "Invalid socket configuration",
      0,
      objectName()
    });

    return;
  }

  // Reset an existing/unfinished connection before applying new config.
  if (!tearDownToUnconnected()) {
    emit logMessage({
      "Failed to reset socket",
      0,
      objectName()
    });

    return;
  }

  m_la = localAddress;
  m_lp = static_cast<quint16>(localPort);
  m_pa = peerAddress;
  m_pp = static_cast<quint16>(peerPort);

  if (!QTcpSocket::bind(m_la, m_lp)) {
    emit logMessage({
      QString("Bind failed: %1").arg(errorString()),
      0,
      objectName()
    });

    return;
  }

  emit logMessage({
    QString("Socket configured:<br/>"
            "&nbsp;&nbsp;Local: &nbsp;[%1] : [%2]<br/>"
            "&nbsp;&nbsp;Peer: &nbsp;&nbsp;[%3] : [%4]")
            .arg(m_la.toString())
            .arg(m_lp)
            .arg(m_pa.toString())
            .arg(m_pp),
    1,
    objectName()
  });

  connectToHost();
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
  writeMessage({ {"cmd", PlcMessageManager::SNAPSHOT} });
}

void SocketDeltaPLC::onReadyRead()
{
  const QByteArray toread = swapBytes(readAll());
  qDebug() << "READ: " << toread.toHex(' ').toUpper();

  PlcMessageManager::ParseResult parsedRespResult = m_mgr.parseMessage(toread, m_nextTid);

  if (!parsedRespResult.ok()) {
    emit logMessage({ "READ ERROR: " + QString::number(parsedRespResult.error), 0, objectName() });
    return;
  }

  emit dataReady(parsedRespResult.data.toMap());
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

QByteArray SocketDeltaPLC::swapBytes(const QByteArray &data)
{
  QByteArray out{data};
  for (int i = 0; i + 1 < out.size(); i += 2)
    qSwap(out[i], out[i + 1]);
  return out;
}




