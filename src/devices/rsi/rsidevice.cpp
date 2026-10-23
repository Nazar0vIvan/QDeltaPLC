#include "devices/rsi/rsidevice.h"
#include "../common/socketconfigutils.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QNetworkDatagram>
#include <QSaveFile>
#include <QTimer>
#include <QUdpSocket>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <chrono>
#include <optional>
#include <utility>

namespace {

constexpr qsizetype TraceCapacity = 64 * 1024 * 1024;

// Fields of a controller <Rob> packet used by RSI_Chamfer_Hole; other elements are ignored.
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
  m_traceClock.start();
  m_traceTimer = new QTimer(this);
  m_traceTimer->setSingleShot(true);
  m_traceTimer->setInterval(250);
  QObject::connect(m_traceTimer, &QTimer::timeout, this, &RsiDevice::saveTrace);
  m_sock = new QUdpSocket(this);
  attachSocket(m_sock);
  QObject::connect(m_sock, &QUdpSocket::readyRead, this, &RsiDevice::onReadyRead);
  // Sleep/wake times of this thread's event loop show why a request was read late.
  // Unique: a restarted device must not record twice per event.
  if (auto* dispatcher = QAbstractEventDispatcher::instance()) {
    QObject::connect(dispatcher, &QAbstractEventDispatcher::aboutToBlock,
                     this, &RsiDevice::onDispatcherAboutToBlock, Qt::UniqueConnection);
    QObject::connect(dispatcher, &QAbstractEventDispatcher::awake,
                     this, &RsiDevice::onDispatcherAwake, Qt::UniqueConnection);
  }
  publishState();
}

void RsiDevice::stopDevice()
{
  if (!m_sock) return;
  disconnect();
  for (auto& save : m_traceSaves) save.wait();
  saveTrace();
  delete std::exchange(m_traceTimer, nullptr);
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
  m_trace.reserve(TraceCapacity); // Allocate before peer traffic can start.
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
  resetRun();
  saveTrace();
  qInfo().noquote() << "[OK]" << objectName() + ":" << "Socket disconnected";
}

void RsiDevice::setCorrections(const QVector<V6d>& increments, const QString& diagnosticId)
{
  // Buffered requests belong to the outgoing stream, not the new array.
  while (m_sock && m_sock->hasPendingDatagrams()) {
    const QNetworkDatagram datagram = m_sock->receiveDatagram();
    if (!acceptsSender(datagram)) continue;
    appendTrace(QByteArray::number(m_traceClock.nsecsElapsed()) + ",rx,-1,-1,-1,"
                + datagram.data().toBase64() + '\n');
  }
  saveTrace();
  if (m_corrections.diagnosticId != diagnosticId) m_tracePart = 0;
  m_corrections = {increments, diagnosticId};
  m_index = 0;
  m_state = RunState::Idle;
  m_pp = 0;
  qInfo().noquote() << "[INFO]" << objectName() + ": RSI corrections installed:"
                    << increments.size();
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
  // No console output before the reply: it can delay it past the controller cycle.
  // Malformed and unanswered requests remain in the packet capture.
  const qint64 entryNs = m_traceClock.nsecsElapsed();
  std::optional<Request> newest;
  QVector<QByteArray> receivedRows;
  while (m_sock && m_sock->hasPendingDatagrams()) {
    const QNetworkDatagram datagram = m_sock->receiveDatagram();
    if (!acceptsSender(datagram)) continue;
    const qint64 receivedNs = m_traceClock.nsecsElapsed();
    receivedRows.append(QByteArray::number(receivedNs) + ",rx,-1,-1,-1,"
                        + datagram.data().toBase64() + '\n');
    const auto request = decodeRequest(datagram.data());
    if (!request) continue;
    m_pp = datagram.senderPort();
    newest = request;
  }
  if (!newest) {
    for (const auto& row : std::as_const(receivedRows)) appendTrace(row);
    return;
  }
  const qsizetype index = m_index;
  Reply reply = advance(newest->start, newest->result);
  reply.ipoc = newest->ipoc;
  const QByteArray encoded = encodeReply(reply);
  const qint64 sentNs = m_traceClock.nsecsElapsed();
  const qint64 written = m_sock->writeDatagram(encoded, m_pa, m_pp);
  const qint64 writeNs = m_traceClock.nsecsElapsed() - sentNs;
  // Capture the whole exchange after replying, outside the reply's measured latency.
  for (const auto& row : std::as_const(receivedRows)) appendTrace(row);
  appendTrace(QByteArray::number(sentNs) + ",tx," + QByteArray::number(index)
              + ',' + QByteArray::number(written) + ',' + QByteArray::number(writeNs)
              + ',' + encoded.toBase64() + ',' + QByteArray::number(entryNs)
              + ',' + QByteArray::number(m_sleepNs) + ',' + QByteArray::number(m_wakeNs)
              + ',' + QByteArray::number(m_handlerDoneNs) + '\n');
  m_handlerDoneNs = m_traceClock.nsecsElapsed();
}

void RsiDevice::onDispatcherAboutToBlock()
{
  m_sleepNs = m_traceClock.nsecsElapsed();
}

void RsiDevice::onDispatcherAwake()
{
  m_wakeNs = m_traceClock.nsecsElapsed();
}

void RsiDevice::appendTrace(const QByteArray& row)
{
  if (row.size() <= TraceCapacity - m_trace.size()) m_trace.append(row);
  else ++m_traceDropped;
  m_traceTimer->start();
}

void RsiDevice::saveTrace()
{
  if (!m_traceTimer) return; // Correction updates may arrive before start or after stop.
  for (auto save = m_traceSaves.begin(); save != m_traceSaves.end();) {
    if (save->wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
      ++save;
      continue;
    }
    qInfo().noquote() << objectName() + ":" << save->get();
    save = m_traceSaves.erase(save);
  }
  m_traceTimer->stop();
  if (!m_trace.isEmpty()) {
    QString id = m_corrections.diagnosticId;
    if (id.isEmpty()) id = QStringLiteral("unassigned_%1").arg(
        QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    ++m_tracePart;
    const QString part = m_tracePart == 1 ? QString{} : QStringLiteral("_part%1").arg(m_tracePart);
    const QString fileName = QStringLiteral("rsi_packets_%1%2.csv").arg(id, part);
    m_trace.append("# plan_id," + m_corrections.diagnosticId.toUtf8()
                   + "\n# dropped_rows," + QByteArray::number(m_traceDropped) + '\n');
    m_traceSaves.push_back(std::async(std::launch::async, &RsiDevice::writeTrace,
                                     std::exchange(m_trace, {}), fileName));
    m_traceDropped = 0;
    if (m_sock->state() == QAbstractSocket::BoundState) m_trace.reserve(TraceCapacity);
  }
  if (!m_traceSaves.empty()) m_traceTimer->start();
}

QString RsiDevice::writeTrace(QByteArray rows, QString fileName)
{
  const QDir records(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("records")));
  QSaveFile file(records.filePath(fileName));
  if (!QDir().mkpath(records.absolutePath()) || !file.open(QIODevice::WriteOnly)) {
    return QStringLiteral("[ERROR] cannot open RSI diagnostic %1 (%2)")
        .arg(file.fileName(), file.errorString());
  }
  const QByteArray header = "# pc_ns: monotonic nanoseconds since device start; XML: exact bytes in Base64\n"
                            "# stream_index: next increment before reply; -1 for received packets\n"
                            "# tx only: handler entry, last event-loop sleep and wake, previous replying handler end (pc_ns)\n"
                            "pc_ns,direction,stream_index,write_bytes,write_ns,xml_base64,"
                            "entry_ns,sleep_ns,wake_ns,prev_done_ns\n";
  if (file.write(header) != header.size() || file.write(rows) != rows.size() || !file.commit()) {
    return QStringLiteral("[ERROR] cannot save RSI diagnostic %1 (%2)")
        .arg(file.fileName(), file.errorString());
  }
  return QStringLiteral("[OK] saved RSI diagnostic %1").arg(file.fileName());
}

RsiDevice::Reply RsiDevice::advance(bool start, qint64 echoedResult)
{
  if (!start) {
    resetRun();
    return {};
  }
  if (m_state == RunState::Idle)
    setState(m_corrections.increments.isEmpty() ? RunState::Stopping : RunState::Streaming);
  if (m_state == RunState::Streaming) {
    Reply reply;
    reply.correction = m_corrections.increments[m_index++];
    if (m_index == m_corrections.increments.size()) setState(RunState::Finished);
    return reply;
  }
  // Stop only after the controller echoes Result = 1, so KRL sees completion.
  if (m_state == RunState::Finished && echoedResult == 1) setState(RunState::Stopping);
  Reply reply;
  reply.stop = m_state == RunState::Stopping;
  // Stopping is reached only from Finished or from Start without a stream.
  reply.result = m_corrections.increments.isEmpty() ? 0 : 1;
  return reply;
}

// Runs on every Start = 0 packet; publishes only an actual change.
void RsiDevice::resetRun()
{
  m_index = 0;
  setState(RunState::Idle);
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
  emit stateReady({{"rsiState", QString::fromLatin1(name)},
                   {"rsiCorrections", m_corrections.increments.size()}});
}
