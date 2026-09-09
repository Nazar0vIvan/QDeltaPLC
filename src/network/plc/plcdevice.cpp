#include "network/plc/plcdevice.h"

#include <QTcpSocket>

PlcDevice::PlcDevice(const QString& name, QObject* parent) : AbstractDevice(name, parent) {}

void PlcDevice::startDevice()
{
  Q_ASSERT(!m_sock);
  Q_ASSERT(!m_mgr);

  m_sock = new QTcpSocket(this);
  m_mgr = new PlcMessageManager(this);

  attachSocket(m_sock);

  QObject::connect(m_sock, &QTcpSocket::connected, this, &PlcDevice::onConnected);
  QObject::connect(m_sock, &QTcpSocket::readyRead, this, &PlcDevice::onReadyRead);
  QObject::connect(m_sock, &QTcpSocket::stateChanged, this, &PlcDevice::onStateChanged);
}

void PlcDevice::stopDevice()
{
  if (!m_sock) return;

  disconnect();

  delete m_sock;
  m_sock = nullptr;

  delete m_mgr;
  m_mgr = nullptr;

  m_rx.clear();
  m_pend.clear();
}

void PlcDevice::connect(const QVariantMap& config)
{
  if (!m_sock) {
    emit logMessage({
      "PLC device is not started",
      0,
      objectName()
    });
    return;
  }

  if (!config.isEmpty()) {
    const QHostAddress addr(config.value("peerAddress").toString());

    bool portOk = false;
    const uint port = config.value("peerPort").toUInt(&portOk);

    if (addr.isNull() || !portOk || port == 0 || port > 65535) {
      emit logMessage({
        "Invalid socket configuration",
        0,
        objectName()
      });
      return;
    }

    m_pa = addr;
    m_pp = static_cast<quint16>(port);
  }

  if (m_pa.isNull() || m_pp == 0) {
    emit logMessage({
      "Peer endpoint is not configured",
      0,
      objectName()
    });
    return;
  }

  if (m_sock->state() != QAbstractSocket::UnconnectedState) {
    m_sock->abort();
  }

  m_sock->connectToHost(m_pa, m_pp, QIODevice::ReadWrite);
}

void PlcDevice::disconnect()
{
  if (!m_sock) return;

  m_sock->disconnectFromHost();
}

void PlcDevice::writeMessage(const QVariantMap& msg)
{
  if (!m_sock || !m_mgr) {
    emit logMessage({
      "PLC device is not started",
      0,
      objectName()
    });
    return;
  }

  const quint8 tid = static_cast<quint8>(m_nextTid + 1);
  const PlcMessageManager::ParseResult built =
      m_mgr->buildReq(msg, tid);

  if (!built.ok()) {
    emit logMessage({
      "WRITE ERROR: " + QString::number(built.error),
      0,
      objectName()
    });
    return;
  }

  m_nextTid = tid;
  m_pend.insert(tid);

  const QByteArray data = built.data.toByteArray();
  const qint64 count = m_sock->write(swapBytes(data));

  emit logMessage({
    count == -1
        ? QStringLiteral("No bytes were written")
        : QString("TX: %1 (%2 bytes)")
            .arg(QString(data.toHex(' ').toUpper()))
            .arg(count),
    count == -1 ? 0 : 4,
    objectName()
  });
}

void PlcDevice::onConnected()
{
  emit logMessage({
    "Connection has been successfully established",
    1,
    objectName()
  });

  writeMessage({
    {"cmd", PlcMessageManager::SNAPSHOT}
  });
}

void PlcDevice::onReadyRead()
{
  m_rx.append(m_sock->readAll());

  while (m_rx.size() >= PlcMessageManager::RESP_SIZE) {
    const QByteArray frame = swapBytes(m_rx.left(PlcMessageManager::RESP_SIZE));

    m_rx.remove(0, PlcMessageManager::RESP_SIZE);

    const PlcMessageManager::ParseResult parsed = m_mgr->parseMessage(frame);

    if (!parsed.ok()) {
      emit logMessage({
        "READ ERROR: " + QString::number(parsed.error),
        0,
        objectName()
      });
      continue;
    }

    const QVariantMap data = parsed.data.toMap();

    if (data.contains("tid")) {
      const quint8 tid = static_cast<quint8>(data.value("tid").toUInt());

      if (!m_pend.remove(tid)) {
        emit logMessage({
          "Unexpected transaction id " + QString::number(tid),
          0,
          objectName()
        });
        continue;
      }
    }

    if (data.value("type").toUInt() == PlcMessageManager::RESP_ERR) {
      emit logMessage({
        QString("PLC response error: cmd=%1, err=%2, code=%3")
          .arg(data.value("cmd").toUInt())
          .arg(data.value("err").toUInt())
          .arg(data.value("code").toUInt()),
          0,
          objectName()
      });

      continue;
    }

    publishData(data);
  }
}

void PlcDevice::onStateChanged(QAbstractSocket::SocketState state)
{
  if (state != QAbstractSocket::UnconnectedState) return;

  m_rx.clear();
  m_pend.clear();

  // Clear the persistent PLC state seen by QML.
  emit stateReady({
    {"x1", QVariantList(8, false)},
    {"y1", QVariantList(8, false)},
    {"x2", QVariantList(8, false)},
    {"y2", QVariantList(8, false)}
  });
}

void PlcDevice::publishData(const QVariantMap& data)
{
  const quint8 type = static_cast<quint8>(data.value("type").toUInt());

  if (type == PlcMessageManager::RESP_OK) {
    const quint8 cmd = static_cast<quint8>(data.value("cmd").toUInt());

    if (cmd == PlcMessageManager::SNAPSHOT) {
      emit stateReady({
        {"x1", data.value("x1")},
        {"y1", data.value("y1")},
        {"x2", data.value("x2")},
        {"y2", data.value("y2")}
      });

      return;
    }

    if (cmd == PlcMessageManager::WRITE_IO) {
      const int mod = data.value("module").toInt();

      if (mod == 1) {
        emit stateReady({{"y1", data.value("state")}});
      } else if (mod == 2) {
        emit stateReady({{"y2", data.value("state")}
        });
      }
      return;
    }
    return;
  }

  if (type == PlcMessageManager::CHG) {
    const quint8 chg = static_cast<quint8>(data.value("chg").toUInt());
    if (chg == PlcMessageManager::IOs) {
      emit stateReady({
        {"x1", data.value("x1")},
        {"y1", data.value("y1")},
        {"x2", data.value("x2")},
        {"y2", data.value("y2")}
      });
      return;
    }

    if (chg == PlcMessageManager::CELL_STATE) {
      emit stateReady({{"cellState", data.value("cellState")}});
    }
  }
}

QByteArray PlcDevice::swapBytes(const QByteArray& data)
{
  QByteArray out = data;

  for (int i = 0; i + 1 < out.size(); i += 2)
    qSwap(out[i], out[i + 1]);

  return out;
}
