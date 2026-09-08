#include "network/fts/ftsdevice.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QTimer>
#include <QUdpSocket>
#include <QtEndian>

#include <cstdlib>
#include <utility>

namespace {

constexpr double kSampleHz = 7000.0;
constexpr double kDt = 1.0 / kSampleHz;
constexpr double kCount = 1'000'000.0;

constexpr int kReqLen = 8;
constexpr int kRespLen = 36;
constexpr int kPulseMs = 300;

bool acceptAxis(qint32& dst, qint32 src, qint64 tol)
{
  const qint64 diff = qint64(src) - qint64(dst);
  if (std::llabs(diff) < tol) return false;

  dst = src;
  return true;
}

} // namespace

FtsDevice::FtsDevice(const QString& name, QObject* parent)
  : AbstractDevice(name, parent) {}

void FtsDevice::startDevice()
{
  Q_ASSERT(!m_sock);
  Q_ASSERT(!m_pulse);

  m_sock = new QUdpSocket(this);
  m_pulse = new QTimer(this);

  m_pulse->setSingleShot(true);
  m_pulse->setInterval(kPulseMs);

  attachSocket(m_sock);

  QObject::connect(
      m_sock,
      &QUdpSocket::readyRead,
      this,
      &FtsDevice::onReadyRead);

  QObject::connect(
      m_pulse,
      &QTimer::timeout,
      this,
      &FtsDevice::onPulseTimeout);

  emit stateReady({{"streaming", false}});
}

void FtsDevice::stopDevice()
{
  if (!m_sock) return;

  disconnect();

  delete std::exchange(m_pulse, nullptr);
  delete std::exchange(m_sock, nullptr);
}

void FtsDevice::connect(const QVariantMap& config)
{
  if (!m_sock) {
    emit logMessage({"FTS device is not started", 0, objectName()});
    return;
  }

  if (!config.isEmpty()) {
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
        || localPort == 0
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

    m_la = localAddress;
    m_lp = static_cast<quint16>(localPort);
    m_pa = peerAddress;
    m_pp = static_cast<quint16>(peerPort);
  }

  if (m_la.isNull()
      || m_lp == 0
      || m_pa.isNull()
      || m_pp == 0) {
    emit logMessage({
      "Socket configuration is incomplete",
      0,
      objectName()
    });
    return;
  }

  if (m_sock->state() != QAbstractSocket::UnconnectedState) disconnect();

  if (!m_sock->bind(
          m_la,
          m_lp,
          QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
    emit logMessage({
      QString("Bind failed: %1").arg(m_sock->errorString()),
      0,
      objectName()
    });
    return;
  }

  emit logMessage({
    QString("Socket connected:<br/>"
            "&nbsp;&nbsp;Local: &nbsp;[%1] : [%2]<br/>"
            "&nbsp;&nbsp;Peer: &nbsp;&nbsp;[%3] : [%4]")
        .arg(m_la.toString())
        .arg(m_lp)
        .arg(m_pa.toString())
        .arg(m_pp),
    1,
    objectName()
  });
}

void FtsDevice::disconnect()
{
  if (!m_sock) return;

  if (m_sock->state() == QAbstractSocket::BoundState
      && !m_pa.isNull()
      && m_pp != 0) {
    stopStreaming();
  }

  m_sock->close();
  setStreaming(false);

  emit logMessage({
    "Socket disconnected",
    1,
    objectName()
  });
}

void FtsDevice::startStreaming()
{
  if (!m_sock) {
    emit logMessage({"FTS device is not started", 0, objectName()});
    return;
  }

  if (m_pa.isNull()) {
    emit logMessage({"Peer address is not set", 0, objectName()});
    return;
  }

  if (m_pp == 0) {
    emit logMessage({"Peer port is not set", 0, objectName()});
    return;
  }

  m_firstRead = true;
  m_baseSeq = 0;
  m_batch.clear();
  m_emitTimer.restart();

  // First LF sample of each stream must reach QML.
  m_hasPub = false;

  emit streamReset();

  sendRequest(0x0002);
}

void FtsDevice::stopStreaming()
{
  setLogRecordingEnabled(false);

  sendRequest(0x0000);

  emit streamReset();
}

void FtsDevice::bias()
{
  sendRequest(0x0042);

  emit streamReset();
}

void FtsDevice::onReadyRead()
{
  while (m_sock && m_sock->hasPendingDatagrams()) {
    const QNetworkDatagram datagram =
        m_sock->receiveDatagram(m_sock->pendingDatagramSize());

    RDTResponse sample = dtg2resp(datagram);

    setStreaming(true);

    if (m_firstRead) {
      m_baseSeq = sample.rdt_sequence;
      m_emitTimer.restart();
      m_batch.clear();
      m_firstRead = false;
    }

    // High-frequency C++ path. This signal does not go through DeviceRunner.
    emit dataSampleHFReady(sample);

    sample.timestamp =
        double(quint32(sample.rdt_sequence - m_baseSeq)) * kDt;

    m_batch.push_back(sample);

    if (m_emitTimer.elapsed() < m_emitMs || m_batch.isEmpty()) continue;

    const RDTResponse lf = m_batch.back();

    appendLogSample(lf);

    emit dataSampleLFReady(lf);
    emit dataBatchReady(m_batch);

    // Low-frequency GUI state.
    publishState(lf);

    m_batch.clear();
    m_emitTimer.restart();
  }
}

void FtsDevice::onPulseTimeout()
{
  setStreaming(false);
}

void FtsDevice::setStreaming(bool enabled)
{
  if (enabled && m_pulse) m_pulse->start();
  if (m_streaming == enabled) return;

  m_streaming = enabled;

  emit stateReady({
    {"streaming", enabled}
  });
}

void FtsDevice::publishState(const RDTResponse& sample)
{
  if (!m_hasPub) {
    m_lastPub = sample;
    m_hasPub = true;

    emit stateReady({
      {"fx", sample.Fx / kCount},
      {"fy", sample.Fy / kCount},
      {"fz", sample.Fz / kCount},
      {"tx", sample.Tx / kCount},
      {"ty", sample.Ty / kCount},
      {"tz", sample.Tz / kCount}
    });

    return;
  }

  const auto tol = static_cast<qint64>(m_tol * kCount);
  QVariantHash vals;

  if (acceptAxis(m_lastPub.Fx, sample.Fx, tol))
    vals.insert("fx", m_lastPub.Fx / kCount);

  if (acceptAxis(m_lastPub.Fy, sample.Fy, tol))
    vals.insert("fy", m_lastPub.Fy / kCount);

  if (acceptAxis(m_lastPub.Fz, sample.Fz, tol))
    vals.insert("fz", m_lastPub.Fz / kCount);

  if (acceptAxis(m_lastPub.Tx, sample.Tx, tol))
    vals.insert("tx", m_lastPub.Tx / kCount);

  if (acceptAxis(m_lastPub.Ty, sample.Ty, tol))
    vals.insert("ty", m_lastPub.Ty / kCount);

  if (acceptAxis(m_lastPub.Tz, sample.Tz, tol))
    vals.insert("tz", m_lastPub.Tz / kCount);

  if (!vals.isEmpty()) emit stateReady(vals);
}

void FtsDevice::sendRequest(quint16 cmd, quint32 count)
{
  if (!m_sock || m_pa.isNull() || m_pp == 0) return;

  const QByteArray data =
      req2dtg(RDTRequest{0x1234, cmd, count}).data();

  m_sock->writeDatagram(data, m_pa, m_pp);
}

QNetworkDatagram FtsDevice::req2dtg(const RDTRequest& request)
{
  QByteArray buffer(kReqLen, 0x00);

  buffer[0] = char(request.header >> 8);
  buffer[1] = char(request.header & 0xff);

  buffer[2] = char(request.command >> 8);
  buffer[3] = char(request.command & 0xff);

  buffer[4] = char(request.sampleCount >> 24);
  buffer[5] = char((request.sampleCount >> 16) & 0xff);
  buffer[6] = char((request.sampleCount >> 8) & 0xff);
  buffer[7] = char(request.sampleCount & 0xff);

  return QNetworkDatagram(buffer);
}

RDTResponse FtsDevice::dtg2resp(const QNetworkDatagram& datagram)
{
  const QByteArray bytes = datagram.data();
  if (bytes.size() < kRespLen) return {};

  const auto* p =
      reinterpret_cast<const uchar*>(bytes.constData());

  RDTResponse response;

  response.rdt_sequence = qFromBigEndian<quint32>(p + 0);
  response.ft_sequence = qFromBigEndian<quint32>(p + 4);
  response.status = qFromBigEndian<quint32>(p + 8);

  response.Fx = qFromBigEndian<qint32>(p + 12);
  response.Fy = qFromBigEndian<qint32>(p + 16);
  response.Fz = qFromBigEndian<qint32>(p + 20);

  response.Tx = qFromBigEndian<qint32>(p + 24);
  response.Ty = qFromBigEndian<qint32>(p + 28);
  response.Tz = qFromBigEndian<qint32>(p + 32);

  return response;
}

void FtsDevice::setLogRecordingEnabled(bool enabled)
{
  if (enabled == m_logEnabled) return;

  if (enabled) {
    if (m_logCap <= 0) {
      emit logMessage({
        "Log capacity must be > 0",
        0,
        objectName()
      });
      return;
    }

    m_logEnabled = true;
    m_log.clear();
    m_log.reserve(m_logCap);

    emit logRecordingEnabledChanged(true);

    emit logMessage({
      QString("LF log recording started (capacity=%1 samples)")
          .arg(m_logCap),
      1,
      objectName()
    });

    return;
  }

  m_logEnabled = false;

  emit logRecordingEnabledChanged(false);

  emit logMessage({
    "LF log recording stopped",
    1,
    objectName()
  });

  if (!m_log.isEmpty()) emit logRecordingReady(m_log);
}

void FtsDevice::startLogRecording()
{
  setLogRecordingEnabled(true);
}

void FtsDevice::stopLogRecording()
{
  setLogRecordingEnabled(false);
}

void FtsDevice::appendLogSample(const RDTResponse& sample)
{
  if (!m_logEnabled) return;

  if (m_logCap <= 0) {
    setLogRecordingEnabled(false);
    return;
  }

  if (m_log.size() < m_logCap) m_log.push_back(sample);

  if (m_log.size() >= m_logCap) {
    emit logMessage({
      QString("LF log reached capacity (%1 samples), auto-stopping")
          .arg(m_logCap),
      2,
      objectName()
    });

    setLogRecordingEnabled(false);
  }
}

void FtsDevice::saveLogToDefaultFile()
{
  saveLogToFileImpl(m_logFile);
}

void FtsDevice::saveLogToFileImpl(const QString& filePath)
{
  QJsonArray samples;

  for (const RDTResponse& sample : std::as_const(m_log)) {
    QJsonObject object;

    object["rdt_sequence"] =
        static_cast<qint64>(sample.rdt_sequence);

    object["ft_sequence"] =
        static_cast<qint64>(sample.ft_sequence);

    object["status"] =
        static_cast<qint64>(sample.status);

    object["Fx"] = static_cast<qint64>(sample.Fx);
    object["Fy"] = static_cast<qint64>(sample.Fy);
    object["Fz"] = static_cast<qint64>(sample.Fz);

    object["Tx"] = static_cast<qint64>(sample.Tx);
    object["Ty"] = static_cast<qint64>(sample.Ty);
    object["Tz"] = static_cast<qint64>(sample.Tz);

    object["timestamp"] = sample.timestamp;

    samples.append(object);
  }

  QJsonObject meta;

  meta["capacity"] = m_logCap;
  meta["count"] = m_log.size();
  meta["emit_interval_ms"] = m_emitMs;
  meta["note"] = QStringLiteral("Low-frequency FTS samples.");

  QJsonObject root;

  root["meta"] = meta;
  root["samples"] = samples;

  const QJsonDocument document(root);
  const QByteArray json =
      document.toJson(QJsonDocument::Indented);

  QFile file(filePath);

  if (!file.open(
          QIODevice::WriteOnly
          | QIODevice::Truncate
          | QIODevice::Text)) {
    emit logMessage({
      QString("Failed to write log file '%1': %2")
          .arg(filePath, file.errorString()),
      0,
      objectName()
    });

    return;
  }

  const qint64 written = file.write(json);
  file.close();

  if (written != json.size()) {
    emit logMessage({
      QString("Partial write to '%1': wrote %2 of %3 bytes")
          .arg(filePath)
          .arg(written)
          .arg(json.size()),
      0,
      objectName()
    });

    return;
  }

  emit logMessage({
    QString("Saved LF log to '%1' (%2 samples, %3 bytes)")
        .arg(filePath)
        .arg(m_log.size())
        .arg(json.size()),
    1,
    objectName()
  });
}