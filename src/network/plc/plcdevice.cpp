#include "network/plc/plcdevice.h"
#include "network/common/socketconfigutils.h"

#include <QDebug>
#include <QTcpSocket>

namespace {

bool matchesResponseField(const QVariantMap& data, const char* key, uint expected)
{
  return data.contains(key) && data.value(key).toUInt() == expected;
}

} // namespace

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
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "PLC device is not started";
    return;
  }

  if (!config.isEmpty()) {
    const QHostAddress localAddr(config.value("localAddress").toString());
    const QHostAddress addr(config.value("peerAddress").toString());

    const auto localPort = parseSocketPort(config.value("localPort"));
    const auto port = parseSocketPort(config.value("peerPort"));

    if (localAddr.isNull() || !localPort || addr.isNull() || !port) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Invalid socket configuration";
      return;
    }

    m_la = localAddr;
    m_lp = *localPort;
    m_pa = addr;
    m_pp = *port;
  }

  if (m_la.isNull() || m_lp == 0 || m_pa.isNull() || m_pp == 0) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Socket configuration is incomplete";
    return;
  }

  if (m_sock->state() != QAbstractSocket::UnconnectedState) {
    m_sock->abort();
  }

  emit stateReady({{"connectionConfig", QVariantMap{
    {"localAddress", m_la.toString()}, {"localPort", m_lp},
    {"peerAddress", m_pa.toString()}, {"peerPort", m_pp}
  }}});

  // The PLC program expects this PC endpoint, including its source port.
  if (!m_sock->bind(m_la, m_lp)) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << QString("Bind failed: %1").arg(m_sock->errorString());
    m_sock->abort();
    return;
  }

  m_sock->connectToHost(m_pa, m_pp, QIODevice::ReadWrite);
}

void PlcDevice::disconnect()
{
  m_rx.clear();
  m_pend.clear();
  if (!m_sock) return;

  m_sock->disconnectFromHost();
}

void PlcDevice::setOutput(const QVariantMap& args)
{
  if (!m_sock || !m_mgr || m_sock->state() != QAbstractSocket::ConnectedState) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "PLC is not connected";
    return;
  }

  const PlcMessageManager::ParseResult request = m_mgr->outputRequest(args);
  if (!request.ok()) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "WRITE ERROR: " + QString::number(request.error);
    return;
  }

  writeMessage(request.data.toMap());
}

void PlcDevice::writeMessage(const QVariantMap& msg)
{
  if (!m_sock || !m_mgr) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "PLC device is not started";
    return;
  }

  const auto available = availableTid();
  if (!available) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "All PLC transaction ids are pending";
    return;
  }
  const quint8 tid = *available;
  const PlcMessageManager::ParseResult built =
      m_mgr->buildReq(msg, tid);

  if (!built.ok()) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "WRITE ERROR: " + QString::number(built.error);
    return;
  }

  m_nextTid = tid;
  m_pend.insert(tid, msg);

  const QByteArray data = built.data.toByteArray();
  const qint64 count = m_sock->write(swapBytes(data));
  if (count != data.size()) {
    m_pend.remove(tid);
    // A partially queued frame cannot safely be followed by another request.
    if (count > 0) m_sock->abort();
  }

  qInfo().noquote() << (count != data.size() ? "[ERROR]" : "[WRITE]") << objectName() + ":"
      << (count != data.size()
        ? QString("Incomplete PLC write: %1 of %2 bytes queued").arg(count).arg(data.size())
        : QString("TX: %1 (%2 bytes)")
            .arg(QString(data.toHex(' ').toUpper()))
            .arg(count));
}

void PlcDevice::onConnected()
{
  qInfo().noquote() << "[OK]" << objectName() + ":" << "Connection has been successfully established";

  writeMessage({
    {"cmd", PlcMessageManager::SNAPSHOT}
  });
}

std::optional<quint8> PlcDevice::availableTid() const
{
  for (int offset = 1; offset <= 256; ++offset) {
    const auto tid = static_cast<quint8>(m_nextTid + offset);
    if (!m_pend.contains(tid)) return tid;
  }
  return std::nullopt;
}

bool PlcDevice::matchResponse(const QVariantMap& data)
{
  const auto tid = static_cast<quint8>(data.value("tid").toUInt());
  const auto pending = m_pend.constFind(tid);
  if (pending == m_pend.cend()) return false;
  const QVariantMap& request = *pending;
  const uint cmd = request.value("cmd").toUInt();
  if (data.value("cmd").toUInt() != cmd) return false;

  // Error replies contain only command/error/code, so no endpoint fields can be matched.
  if (data.value("type").toUInt() == PlcMessageManager::RESP_OK) {
    switch (cmd) {
      case PlcMessageManager::READ_IO:
      case PlcMessageManager::WRITE_IO:
        if (!matchesResponseField(data, "dev", cmd == PlcMessageManager::WRITE_IO
                             ? uint(PlcMessageManager::Y) : request.value("dev").toUInt())
            || !matchesResponseField(data, "module", request.value("module").toUInt())) return false;
        break;
      case PlcMessageManager::READ_REG:
      case PlcMessageManager::WRITE_REG:
        if (!matchesResponseField(data, "dev", PlcMessageManager::D)
            || !matchesResponseField(data, "addr", request.value("addr").toUInt())) return false;
        if (cmd == PlcMessageManager::WRITE_REG
            && !matchesResponseField(data, "value", request.value("value").toUInt())) return false;
        break;
      case PlcMessageManager::SET_VAR:
        if (!matchesResponseField(data, "var", request.value("var").toUInt())
            || !matchesResponseField(data, "attr", request.value("attr").toUInt())) return false;
        break;
      default: break;
    }
  }
  m_pend.remove(tid);
  return true;
}

void PlcDevice::onReadyRead()
{
  processIncoming(m_sock->readAll());
}

void PlcDevice::processIncoming(const QByteArray& bytes)
{
  m_rx.append(bytes);

  while (m_rx.size() >= PlcMessageManager::RESP_SIZE) {
    const QByteArray frame = swapBytes(m_rx.left(PlcMessageManager::RESP_SIZE));

    m_rx.remove(0, PlcMessageManager::RESP_SIZE);

    const PlcMessageManager::ParseResult parsed = m_mgr->parseMessage(frame);

    if (!parsed.ok()) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":" << "READ ERROR: " + QString::number(parsed.error);
      continue;
    }

    const QVariantMap data = parsed.data.toMap();

    if (data.contains("tid")) {
      if (!matchResponse(data)) {
        qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Unmatched PLC response for transaction " + data.value("tid").toString();
        continue;
      }
    }

    if (data.value("type").toUInt() == PlcMessageManager::RESP_ERR) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":"
          << QString("PLC response error: cmd=%1, err=%2, code=%3")
                 .arg(data.value("cmd").toUInt())
                 .arg(data.value("err").toUInt())
                 .arg(data.value("code").toUInt());

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
