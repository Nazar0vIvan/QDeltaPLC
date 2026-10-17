#include "devices/rsi/rsidevice.h"
#include "../common/socketconfigutils.h"

#include <QDebug>
#include <QNetworkDatagram>
#include <QUdpSocket>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <utility>

namespace {

// Fields of a controller <Rob> packet used by RSI_Hole; other elements are ignored.
struct Request
{
  quint64 ipoc = 0;
  bool start = false;
  qint64 result = 0;
};

bool isDigits(const QString& text)
{
  if (text.isEmpty()) return false;
  for (const QChar character : text)
    if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
  return true;
}

// Requires exactly one IPOC, Start (0/1) and Result (LONG) in a well-formed <Rob> document.
std::optional<Request> decodeRequest(const QByteArray& datagram)
{
  QXmlStreamReader xml(datagram);
  if (!xml.readNextStartElement() || xml.name() != QLatin1String("Rob")) return std::nullopt;
  Request request;
  bool hasIpoc = false;
  bool hasStart = false;
  bool hasResult = false;
  while (xml.readNextStartElement()) {
    if (xml.name() == QLatin1String("IPOC")) {
      const QString text = xml.readElementText().trimmed();
      bool ok = false;
      request.ipoc = text.toULongLong(&ok);
      if (hasIpoc || !isDigits(text) || !ok) return std::nullopt;
      hasIpoc = true;
    } else if (xml.name() == QLatin1String("Start")) {
      const QString text = xml.readElementText().trimmed();
      if (hasStart || (text != QLatin1String("0") && text != QLatin1String("1"))) return std::nullopt;
      request.start = text == QLatin1String("1");
      hasStart = true;
    } else if (xml.name() == QLatin1String("Result")) {
      bool ok = false;
      request.result = xml.readElementText().trimmed().toLongLong(&ok);
      if (hasResult || !ok) return std::nullopt;
      hasResult = true;
    } else {
      xml.skipCurrentElement();
    }
  }
  // Reading only through </Rob> misses truncated documents and trailing garbage.
  while (!xml.atEnd()) xml.readNext();
  if (xml.hasError() || !hasIpoc || !hasStart || !hasResult) return std::nullopt;
  return request;
}

// Fixed notation: no exponent text such as 1e-05 reaches the controller.
QByteArray encodeReply(const RsiDevice::Reply& reply)
{
  static const char* const axes[6] = {"X", "Y", "Z", "A", "B", "C"};
  QByteArray datagram;
  QXmlStreamWriter xml(&datagram);
  xml.writeStartElement(QStringLiteral("Sen"));
  xml.writeAttribute(QStringLiteral("Type"), QStringLiteral("ImFree"));
  xml.writeEmptyElement(QStringLiteral("RKorr"));
  for (int axis = 0; axis < 6; ++axis)
    xml.writeAttribute(QLatin1String(axes[axis]), QString::number(reply.correction[axis], 'f', 6));
  xml.writeTextElement(QStringLiteral("Stop"), reply.stop ? QStringLiteral("1") : QStringLiteral("0"));
  xml.writeTextElement(QStringLiteral("Result"), QString::number(reply.result));
  xml.writeTextElement(QStringLiteral("IPOC"), QString::number(reply.ipoc));
  xml.writeEndElement();
  return datagram;
}

} // namespace

RsiDevice::RsiDevice(const QString& name, QObject* parent) : AbstractDevice(name, parent) {}

void RsiDevice::startDevice()
{
  Q_ASSERT(!m_sock);
  m_sock = new QUdpSocket(this);
  attachSocket(m_sock);
  QObject::connect(m_sock, &QUdpSocket::readyRead, this, &RsiDevice::onReadyRead);
  publishState();
}

void RsiDevice::stopDevice()
{
  if (!m_sock) return;
  disconnect();
  delete std::exchange(m_sock, nullptr);
}

void RsiDevice::connect(const QVariantMap& config)
{
  if (!m_sock) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "RSI device is not started";
    return;
  }
  if (!config.isEmpty()) {
    const QHostAddress la(config.value("localAddress").toString());
    const QHostAddress pa(config.value("peerAddress").toString());
    const auto lp = parseSocketPort(config.value("localPort"));
    if (la.isNull() || pa.isNull() || !lp) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Invalid socket configuration";
      return;
    }
    m_la = la;
    m_lp = *lp;
    m_pa = pa;
  }
  if (m_la.isNull() || m_lp == 0 || m_pa.isNull()) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Socket configuration is incomplete";
    return;
  }
  if (m_sock->state() != QAbstractSocket::UnconnectedState) disconnect();
  emit stateReady({{"connectionConfig", QVariantMap{
    {"localAddress", m_la.toString()}, {"localPort", m_lp},
    {"peerAddress", m_pa.toString()}
  }}});
  if (!m_sock->bind(m_la, m_lp)) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << QString("Bind failed: %1").arg(m_sock->errorString());
    return;
  }
  qInfo().noquote() << "[OK]" << objectName() + ":"
      << QString("Socket connected: %1:%2").arg(m_la.toString()).arg(m_lp);
}

void RsiDevice::disconnect()
{
  if (!m_sock) return;
  m_sock->close();
  // The controller peer port is learned again from the next connection.
  m_pp = 0;
  m_decodeFailureLogged = false;
  resetRun();
  qInfo().noquote() << "[OK]" << objectName() + ":" << "Socket disconnected";
}

void RsiDevice::setCorrections(const QVector<V6d>& increments)
{
  if (m_state != RunState::Idle) {
    m_pending = increments;
    return;
  }
  m_corrections = increments;
  publishState();
}

bool RsiDevice::acceptsSender(const QNetworkDatagram& datagram) const
{
  return !m_pa.isNull() && datagram.senderAddress() == m_pa && datagram.senderPort() > 0
      && (m_pp == 0 || datagram.senderPort() == m_pp);
}

void RsiDevice::onReadyRead()
{
  // Replies to older queued packets would arrive late and be rejected, losing
  // their corrections; answer only the newest so the path is delayed, not cut.
  std::optional<Request> newest;
  int accepted = 0;
  while (m_sock && m_sock->hasPendingDatagrams()) {
    const QNetworkDatagram datagram = m_sock->receiveDatagram();
    if (!acceptsSender(datagram)) continue;
    const auto request = decodeRequest(datagram.data());
    if (!request) {
      if (!std::exchange(m_decodeFailureLogged, true))
        qInfo().noquote() << "[ERROR]" << objectName() + ": cannot decode RSI packet:"
                          << QString::fromUtf8(datagram.data().left(200));
      continue;
    }
    m_pp = datagram.senderPort();
    newest = request;
    ++accepted;
  }
  if (!newest) return;
  if (accepted > 1)
    qInfo().noquote() << "[INFO]" << objectName() + ":"
        << QStringLiteral("answered newest of %1 queued RSI requests").arg(accepted);
  Reply reply = advance(newest->start, newest->result);
  reply.ipoc = newest->ipoc;
  m_sock->writeDatagram(encodeReply(reply), m_pa, m_pp);
}

RsiDevice::Reply RsiDevice::advance(bool start, qint64 echoedResult)
{
  if (!start) {
    resetRun();
    return {};
  }
  if (m_state == RunState::Idle) {
    if (m_corrections.isEmpty()) {
      qInfo().noquote() << "[ERROR]" << objectName() + ": Start without a sampled RSI stream";
      setState(RunState::Stopping);
    } else {
      setState(RunState::Streaming);
    }
  }
  if (m_state == RunState::Streaming) {
    Reply reply;
    reply.correction = m_corrections[m_index++];
    if (m_index == m_corrections.size()) setState(RunState::Finished);
    return reply;
  }
  // Stop only after the controller echoes Result = 1, so KRL sees completion.
  if (m_state == RunState::Finished && echoedResult == 1) setState(RunState::Stopping);
  Reply reply;
  reply.stop = m_state == RunState::Stopping;
  // Stopping is reached only from Finished or from Start without a stream.
  reply.result = m_corrections.isEmpty() ? 0 : 1;
  return reply;
}

// Runs on every Start = 0 packet; publishes only an actual change.
void RsiDevice::resetRun()
{
  m_index = 0;
  const bool replaced = m_pending.has_value();
  if (replaced) m_corrections = *std::exchange(m_pending, std::nullopt);
  if (m_state != RunState::Idle) setState(RunState::Idle);
  else if (replaced) publishState();
}

void RsiDevice::setState(RunState state)
{
  if (m_state == state) return;
  m_state = state;
  publishState();
}

void RsiDevice::publishState()
{
  static const char* const names[4] = {"idle", "streaming", "finished", "stopping"};
  const char* name = names[static_cast<int>(m_state)];
  qInfo().noquote() << "[INFO]" << objectName() + ": RSI" << name << "·"
                    << m_corrections.size() << "corrections";
  emit stateReady({{"rsiState", QString::fromLatin1(name)},
                   {"rsiCorrections", m_corrections.size()}});
}
