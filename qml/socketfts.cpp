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



