#include "socketfts.h"

#include <QFile>
#include <QIODeviceBase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QtEndian>

#include "network/common/socketstateutils.h"

namespace {

constexpr double kSampleHz = 7000.0;
constexpr double kDt = 1.0 / kSampleHz;

constexpr int kRdtRequestLength = 8;
constexpr int kRdtResponseLength = 36;

} // namespace

SocketFTS::SocketFTS(const QString& name, QObject* parent) : QUdpSocket(parent)
{
  setObjectName(name);
  setOpenMode(QIODeviceBase::ReadWrite);

  connect(this, &SocketFTS::readyRead, this, &SocketFTS::onReadyRead);
  connect(this, &SocketFTS::errorOccurred, this, &SocketFTS::onErrorOccurred);
  connect(this, &SocketFTS::stateChanged, this, &SocketFTS::onStateChanged);
  connect(this, &SocketFTS::logMessage, Logger::instance(), &Logger::push);
}

void SocketFTS::startStreaming()
{
  if (m_pa.isNull()) {
    emit logMessage({"Peer address is not set", 0, objectName()});
    return;
  }

  if (m_pp == 0) {
    emit logMessage({"Peer port is not set", 0, objectName()});
    return;
  }

  m_isFirstRead = true;
  m_baseSeq = 0;
  m_batch.clear();
  m_emitTimer.restart();

  emit streamReset();

  const QByteArray request = req2dtg(RDTRequest{0x1234, 0x0002, 0}).data();
  writeDatagram(request, m_pa, m_pp);
}

void SocketFTS::stopStreaming()
{
  setLogRecordingEnabled(false);

  const QByteArray request = req2dtg(RDTRequest{0x1234, 0x0000, 0}).data();
  writeDatagram(request, m_pa, m_pp);

  emit streamReset();
}

void SocketFTS::bias()
{
  const QByteArray request = req2dtg(RDTRequest{0x1234, 0x0042, 0}).data();
  writeDatagram(request, m_pa, m_pp);

  emit streamReset();
}

void SocketFTS::setSocketConfig(const QVariantMap& config)
{
  m_la = QHostAddress(config.value("localAddress").toString());
  m_lp = config.value("localPort").toUInt();

  m_pa = QHostAddress(config.value("peerAddress").toString());
  m_pp = config.value("peerPort").toUInt();

  if (state() != QAbstractSocket::BoundState) {
    if (!bind(m_la, m_lp, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
      emit logMessage({QString("Bind failed: %1").arg(errorString()), 0, objectName()});
      return;
    }
  }

  emit logMessage({
    QString("Socket configured:<br/>&nbsp;&nbsp;Local: &nbsp;%1:%2<br/>&nbsp;&nbsp;Peer: &nbsp;&nbsp;%3:%4")
      .arg(m_la.toString())
      .arg(m_lp)
      .arg(m_pa.toString())
      .arg(m_pp),
      1,
      objectName()
    });
}

void SocketFTS::onReadyRead()
{
  while (hasPendingDatagrams()) {
    const QNetworkDatagram datagram = receiveDatagram(pendingDatagramSize());
    RDTResponse sample = dtg2resp(datagram);

    if (m_isFirstRead) {
      m_baseSeq = sample.rdt_sequence;
      m_emitTimer.restart();
      m_batch.clear();
      m_isFirstRead = false;
    }

    emit dataSampleHFReady(sample);

    sample.timestamp = double(quint32(sample.rdt_sequence - m_baseSeq)) * kDt;

    m_batch.push_back(sample);

    if (m_emitTimer.elapsed() >= m_emitIntervalMs && !m_batch.isEmpty()) {
      const RDTResponse lf = m_batch.back();

      appendLogSample(lf);

      emit dataSampleLFReady(lf);
      emit dataBatchReady(m_batch);

      m_batch.clear();
      m_emitTimer.restart();
    }
  }
}

void SocketFTS::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
  emit logMessage({
    QString("%1: %2").arg(socketErrorName(socketError), errorString()),
    0,
    objectName()
  });
}

void SocketFTS::onStateChanged(QAbstractSocket::SocketState state)
{
  emit logMessage({socketStateName(state), 2, objectName()});
}

QNetworkDatagram SocketFTS::req2dtg(const RDTRequest& request)
{
  QByteArray buffer(kRdtRequestLength, 0x00);

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

RDTResponse SocketFTS::dtg2resp(const QNetworkDatagram& datagram)
{
  const QByteArray bytes = datagram.data();

  if (bytes.size() < kRdtResponseLength) {
    return {};
  }

  const auto* p = reinterpret_cast<const uchar*>(bytes.constData());

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

void SocketFTS::setLogRecordingEnabled(bool enabled)
{
  if (enabled == m_logEnabled) return;

  if (enabled) {
    if (m_logCapacity <= 0) {
      emit logMessage({"Log capacity must be > 0", 0, objectName()});
      return;
    }

    m_logEnabled = true;
    m_log.clear();
    m_log.reserve(m_logCapacity);

    emit logRecordingEnabledChanged(true);
    emit logMessage({QString("LF log recording started (capacity=%1 samples)").arg(m_logCapacity), 1, objectName()});

    return;
  }

  m_logEnabled = false;

  emit logRecordingEnabledChanged(false);
  emit logMessage({"LF log recording stopped", 1, objectName()});

  if (!m_log.isEmpty()) {
    emit logRecordingReady(m_log);
  }
}

void SocketFTS::startLogRecording()
{
  setLogRecordingEnabled(true);
}

void SocketFTS::stopLogRecording()
{
  setLogRecordingEnabled(false);
}

void SocketFTS::appendLogSample(const RDTResponse& sample)
{
  if (!m_logEnabled) return;

  if (m_logCapacity <= 0) {
    setLogRecordingEnabled(false);
    return;
  }

  if (m_log.size() < m_logCapacity) {
    m_log.push_back(sample);
  }

  if (m_log.size() >= m_logCapacity) {
    emit logMessage({QString("LF log reached capacity (%1 samples), auto-stopping").arg(m_logCapacity), 2, objectName()});
    setLogRecordingEnabled(false);
  }
}

QVector<RDTResponse> SocketFTS::exportLogSamples() const
{
  return m_log;
}

void SocketFTS::clearLog()
{
  m_log.clear();
}

void SocketFTS::saveLogToDefaultFile()
{
  return saveLogToFileImpl(m_logFilePath);
}

void SocketFTS::saveLogToFileImpl(const QString& filePath)
{
  QJsonArray samples;

  for (const RDTResponse& sample : std::as_const(m_log)) {
    QJsonObject object;

    object["rdt_sequence"] = static_cast<qint64>(sample.rdt_sequence);
    object["ft_sequence"] = static_cast<qint64>(sample.ft_sequence);
    object["status"] = static_cast<qint64>(sample.status);

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
  meta["capacity"] = m_logCapacity;
  meta["count"] = m_log.size();
  meta["emit_interval_ms"] = m_emitIntervalMs;
  meta["note"] = QStringLiteral("Low-frequency samples captured from dataSampleLFReady.");

  QJsonObject root;
  root["meta"] = meta;
  root["samples"] = samples;

  const QJsonDocument document(root);
  const QByteArray json = document.toJson(QJsonDocument::Indented);

  QFile file(filePath);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    emit logMessage({QString("Failed to write log file '%1': %2").arg(filePath, file.errorString()), 0, objectName()});
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
