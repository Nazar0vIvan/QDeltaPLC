#include "socketfts.h"

static constexpr double SAMPLE_HZ = 7000.0;
static constexpr double DT = 1.0 / SAMPLE_HZ;
static constexpr int RDT_REQUEST_LENGTH = 8;
static constexpr int RDT_RESPONSE_LENGTH = 36;

SocketFTS::SocketFTS(const QString& name, QObject* parent) : QUdpSocket(parent)
{
  this->setObjectName(name);

  setOpenMode(QIODeviceBase::ReadWrite);

  connect(this, &SocketFTS::readyRead, this, &SocketFTS::onReadyRead);
  connect(this, &SocketFTS::errorOccurred, this, &SocketFTS::onErrorOccurred);
  connect(this, &SocketFTS::stateChanged,  this, &SocketFTS::onStateChanged);
  connect(this, &SocketFTS::logMessage,  Logger::instance(), &Logger::push);
}

// Q_INVOKABLE

void SocketFTS::startStreaming()
{
  if (m_pa.isNull()) {
    emit logMessage({"Peer address is not set", 0, objectName()});
    return;
  }
  if (!m_pp) {
    emit logMessage({"Peer port is not set", 0, objectName()});
    return;
  }
  // reset batching/timeline
  m_isFirstRead = true;
  m_baseSeq = 0;
  m_batch.clear();
  m_emitTimer.restart();
  emit streamReset(); // tell GUI to clear immediately

  const QByteArray startReq = req2dtg(RDTRequest{0x1234,0x0002,0}).data();
  writeDatagram(startReq, m_pa, m_pp);
}

void SocketFTS::stopStreaming()
{
  setLogRecordingEnabled(false);

  const QByteArray stopReq = req2dtg(RDTRequest{0x1234,0x0000,0}).data();
  writeDatagram(stopReq, m_pa, m_pp);
  emit streamReset();
}

void SocketFTS::setSocketConfig(const QVariantMap &config)
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

  emit logMessage({QString("Socket configured:<br/>"
                   "&nbsp;&nbsp;Local: &nbsp;%1:%2<br/>"
                   "&nbsp;&nbsp;Peer: &nbsp;&nbsp;%3:%4").
                   arg(m_la.toString()).arg(m_lp).arg(m_pa.toString()).arg(m_pp),
                   1, objectName()});
}

// PUBLIC SLOTS

void SocketFTS::onReadyRead()
{
  do {
    QNetworkDatagram dg = receiveDatagram(pendingDatagramSize());
    RDTResponse sample = dtg2resp(dg);

    if (m_isFirstRead) { // if the first read
      m_baseSeq = sample.rdt_sequence;
      m_emitTimer.restart();
      m_batch.clear();
      m_isFirstRead = false;
    }

    emit dataSampleHFReady(sample);

    const double t = double(quint32(sample.rdt_sequence - m_baseSeq)) * DT;
    sample.timestamp = t;

    m_batch.push_back(sample);
    if (m_emitTimer.elapsed() >= m_emitIntervalMs && !m_batch.isEmpty()) {
      const RDTResponse lf = m_batch.back();
      appendLogSample(lf);

      emit dataSampleLFReady(m_batch.back());
      emit dataBatchReady(std::exchange(m_batch, {}));
      m_emitTimer.restart();
    }

  } while(hasPendingDatagrams());
}

void SocketFTS::onErrorOccurred(QAbstractSocket::SocketError socketError) {
  emit logMessage({this->errorString(), 0, objectName()});
}

void SocketFTS::onStateChanged(QAbstractSocket::SocketState state) {
  emit logMessage({stateToString(state), 2, objectName()});
}

// PRIVATE
QString SocketFTS::stateToString(SocketState state)
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

QNetworkDatagram SocketFTS::req2dtg(const RDTRequest& request)
{
  QByteArray buffer(RDT_REQUEST_LENGTH, 0x00);

  buffer[0] = (unsigned char)(request.header >> 8);
  buffer[1] = (unsigned char)(request.header & 0xff);
  buffer[2] = (unsigned char)(request.command >> 8);
  buffer[3] = (unsigned char)(request.command & 0xff);
  buffer[4] = (unsigned char)(request.sampleCount >> 24);
  buffer[5] = (unsigned char)((request.sampleCount >> 16) & 0xff);
  buffer[6] = (unsigned char)((request.sampleCount >> 8) & 0xff);
  buffer[7] = (unsigned char)(request.sampleCount & 0xff);

  return QNetworkDatagram(buffer);
}

RDTResponse SocketFTS::dtg2resp(const QNetworkDatagram& dtg) const
{
  const QByteArray bytes = dtg.data();
  if (bytes.size() < RDT_RESPONSE_LENGTH) {
    return {};
  }

  const uchar* p = reinterpret_cast<const uchar*>(bytes.constData());

  RDTResponse r{};
  r.rdt_sequence = qFromBigEndian<quint32>(p + 0);
  r.ft_sequence  = qFromBigEndian<quint32>(p + 4);
  r.status       = qFromBigEndian<quint32>(p + 8);
  r.Fx           = qFromBigEndian<qint32>(p + 12);
  r.Fy           = qFromBigEndian<qint32>(p + 16);
  r.Fz           = qFromBigEndian<qint32>(p + 20);
  r.Tx           = qFromBigEndian<qint32>(p + 24);
  r.Ty           = qFromBigEndian<qint32>(p + 28);
  r.Tz           = qFromBigEndian<qint32>(p + 32);
  return r;
}

// samples logger

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
    emit logMessage({QString("LF log recording started (capacity=%1 samples)").arg(m_logCapacity),
                     1, objectName()});
    return;
  }

  // disabling
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

void SocketFTS::appendLogSample(const RDTResponse &sample)
{
  if (!m_logEnabled) {
    return;
  }

  if (m_logCapacity <= 0) {
    setLogRecordingEnabled(false);
    return;
  }

  if (m_log.size() < m_logCapacity) {
    m_log.push_back(sample);
  }

  // Stop exactly when capacity is reached
  if (m_log.size() >= m_logCapacity) {
    emit logMessage({QString("LF log reached capacity (%1 samples), auto-stopping").arg(m_logCapacity),
                     2, objectName()});
    setLogRecordingEnabled(false);
  }
}

void SocketFTS::clearLog()
{
  m_log.clear();
}

bool SocketFTS::saveLogToDefaultFile()
{
  return saveLogToFileImpl(m_logFilePath);
}

bool SocketFTS::saveLogToFileImpl(const QString &filePath)
{
  QJsonArray samplesArr;

  for (const auto& s : std::as_const(m_log)) {
    QJsonObject o;
    o["rdt_sequence"] = static_cast<qint64>(s.rdt_sequence);
    o["ft_sequence"]  = static_cast<qint64>(s.ft_sequence);
    o["status"]       = static_cast<qint64>(s.status);
    o["Fx"]           = static_cast<qint64>(s.Fx);
    o["Fy"]           = static_cast<qint64>(s.Fy);
    o["Fz"]           = static_cast<qint64>(s.Fz);
    o["Tx"]           = static_cast<qint64>(s.Tx);
    o["Ty"]           = static_cast<qint64>(s.Ty);
    o["Tz"]           = static_cast<qint64>(s.Tz);
    o["timestamp"]    = s.timestamp;

    samplesArr.append(o);
  }
  QJsonObject meta;
  meta["capacity"] = m_logCapacity;
  meta["count"]    = m_log.size();
  meta["emit_interval_ms"] = m_emitIntervalMs;
  meta["note"] = QString("Low-frequency samples captured from dataSampleLFReady; overwritten each session.");

  QJsonObject root;
  root["meta"] = meta;
  root["samples"] = samplesArr;

  const QJsonDocument doc(root);
  const QByteArray json = doc.toJson(QJsonDocument::Indented);

  QFile f(filePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    emit logMessage({QString("Failed to write log file '%1': %2").arg(filePath, f.errorString()),
                     0, objectName()});
    return false;
  }

  const qint64 written = f.write(json);
  f.close();

  if (written != json.size()) {
    emit logMessage({QString("Partial write to '%1': wrote %2 of %3 bytes")
                       .arg(filePath).arg(written).arg(json.size()),
                     0, objectName()});
    return false;
  }

  emit logMessage({QString("Saved LF log to '%1' (%2 samples, %3 bytes)")
                       .arg(filePath).arg(m_log.size()).arg(json.size()),
                   1, objectName()});
  return true;
}

