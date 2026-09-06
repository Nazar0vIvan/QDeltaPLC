#include "socketdeltaplc.h"

SocketDeltaPLC::SocketDeltaPLC(const QString& name, QObject* parent) : QTcpSocket(parent)
{
  this->setObjectName(name);

  QObject::connect(this, &SocketDeltaPLC::errorOccurred, this, &SocketDeltaPLC::onErrorOccurred);
  QObject::connect(this, &SocketDeltaPLC::stateChanged,  this, &SocketDeltaPLC::onStateChanged);
  QObject::connect(this, &SocketDeltaPLC::connected,     this, &SocketDeltaPLC::onConnected);
  QObject::connect(this, &SocketDeltaPLC::readyRead,     this, &SocketDeltaPLC::onReadyRead);

  QObject::connect(this, &SocketDeltaPLC::logMessage, Logger::instance(), &Logger::push);
}

void SocketDeltaPLC::connect(const QVariantMap& config)
{
  if (!config.isEmpty()) {
    const QHostAddress peerAddress(config.value("peerAddress").toString());
    bool peerPortOk = false;
    const uint peerPort = config.value("peerPort").toUInt(&peerPortOk);

    if (peerAddress.isNull()
      || !peerPortOk
      || peerPort == 0
      || peerPort > 65535) {

      emit logMessage({
        "Invalid socket configuration",
        0,
        objectName()
      });

      return;
    }

    m_pa = peerAddress;
    m_pp = static_cast<quint16>(peerPort);
  }

  if (m_pa.isNull() || m_pp == 0) {
    emit logMessage({
      "Peer endpoint is not configured",
      0,
      objectName()
    });

    return;
  }

  if (!tearDownToUnconnected()) {
    emit logMessage({
      "Failed to reset socket",
      0,
      objectName()
    });

    return;
  }

  // TCP: no local bind.
  QTcpSocket::connectToHost(m_pa,m_pp, QIODevice::ReadWrite);
}

void SocketDeltaPLC::disconnect()
{
  QTcpSocket::disconnectFromHost();
}

SocketDeltaPLC::~SocketDeltaPLC() {}

// Q_INVOKABLE


void SocketDeltaPLC::writeMessage(const QVariantMap& msg)
{
	const quint8 tid = static_cast<quint8>(m_nextTid + 1);
	const PlcMessageManager::ParseResult built = m_mgr.buildReq(msg, tid);

	if (!built.ok()) {
		emit logMessage({ "WRITE ERROR: " + QString::number(built.error), 0, objectName() });
		return;
	}

	m_nextTid = tid;
	m_pend.insert(tid);

	QByteArray tosend = built.data.toByteArray();
  const qint64 n = write(swapBytes(tosend));

  emit logMessage({ (n == -1 ? "No bytes were written" :
                    "TX: " + tosend.toHex(' ').toUpper() + " (" + QString::number(n) + " bytes)"),
                    (n == -1 ? 0 : 4), objectName()});
}

// PUBLIC SLOTS

void SocketDeltaPLC::onErrorOccurred(QAbstractSocket::SocketError socketError) {
  emit logMessage({this->errorString(), 0, objectName()});
}

void SocketDeltaPLC::onStateChanged(QAbstractSocket::SocketState state) {
	if (state == QAbstractSocket::UnconnectedState) {
		m_rx.clear();
		m_pend.clear();
	}
	emit logMessage({stateToString(state), 2, objectName()});
}

void SocketDeltaPLC::onConnected()
{
  emit logMessage({"Connection has been successfully established", 1, objectName()});
  writeMessage({ {"cmd", PlcMessageManager::SNAPSHOT} });
}

void SocketDeltaPLC::onReadyRead()
{
	m_rx.append(readAll());

	while (m_rx.size() >= PlcMessageManager::RESP_SIZE) {
		const QByteArray frame = swapBytes(m_rx.left(PlcMessageManager::RESP_SIZE));
		m_rx.remove(0, PlcMessageManager::RESP_SIZE);

		const PlcMessageManager::ParseResult parsed = m_mgr.parseMessage(frame);

		if (!parsed.ok()) {
			emit logMessage({ "READ ERROR: " + QString::number(parsed.error), 0, objectName() });
			continue;
		}

		const QVariantMap data = parsed.data.toMap();

		if (data.contains("tid")) {
			const quint8 tid = static_cast<quint8>(data.value("tid").toUInt());
			if (!m_pend.remove(tid)) {
				emit logMessage({ "Unexpected transaction id " + QString::number(tid), 0, objectName() });
				continue;
			}
		}

		emit dataReady(data);
	}
}

// PRIVATE

bool SocketDeltaPLC::tearDownToUnconnected(int ms)
{
  if (state() == QAbstractSocket::UnconnectedState)
    return true;

  QTcpSocket::disconnectFromHost();
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




