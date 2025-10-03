#include "socketrdt.h"

static constexpr double SAMPLE_HZ = 7000.0;
static constexpr double DT = 1.0 / SAMPLE_HZ;
static constexpr int COUNT_FACTOR = 1000000;

SocketRDT::SocketRDT(const QString& name, QObject* parent) : QUdpSocket(parent)
{
    this->setObjectName(name);

    setLocalPort(RDT_LOCAL_PORT);
    setPeerPort(RDT_PEER_PORT);
    setOpenMode(QIODeviceBase::ReadWrite);

    connect(this, &SocketRDT::readyRead, this, &SocketRDT::onReadyRead);
    connect(this, &SocketRDT::errorOccurred, this, &SocketRDT::onErrorOccurred);
    connect(this, &SocketRDT::stateChanged,  this, &SocketRDT::onStateChanged);
    connect(this, &SocketRDT::logMessage,  Logger::instance(), &Logger::push);
}

void SocketRDT::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    emit logMessage({this->errorString(), 0, objectName()});
}

void SocketRDT::onStateChanged(QAbstractSocket::SocketState state) {
    emit logMessage({stateToString(state), 2, objectName()});
}

QString SocketRDT::stateToString(SocketState state)
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

QNetworkDatagram SocketRDT::RDTRequest2QNetworkDatagram(const RDTRequest& request)
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

QVariantList SocketRDT::QNetworkDatagram2RDTResponse(const QNetworkDatagram& networkDatagram)
{
    QByteArray bytes(networkDatagram.data());

    return {
        qFromBigEndian<uint32_t>(bytes.left(4).data()),
        qFromBigEndian<uint32_t>(bytes.right(32).left(4).data()),
        qFromBigEndian<uint32_t>(bytes.right(28).left(4).data()),
        qFromBigEndian<int32_t>(bytes.right(24).left(4).data()),
        qFromBigEndian<int32_t>(bytes.right(20).left(4).data()),
        qFromBigEndian<int32_t>(bytes.right(16).left(4).data()),
        qFromBigEndian<int32_t>(bytes.right(12).left(4).data()),
        qFromBigEndian<int32_t>(bytes.right(8).left(4).data()),
        qFromBigEndian<int32_t>(bytes.right(4).data()),
    };
}

void SocketRDT::startStreaming(const QVariantMap& data)
{
    QHostAddress pa = QHostAddress(data.value("peerAddress").toString());
    qint16 pp = data.value("peerPort").toUInt();

    setPeerAddress(pa); setPeerPort(pp);

    // reset batching/timeline
    m_haveBase = false;
    m_baseSeq = 0;
    m_readings.clear();
    m_emitTimer.restart();
    emit streamReset(); // tell GUI to clear immediately

    const QByteArray startReq = RDTRequest2QNetworkDatagram(RDTRequest{0x1234,0x0002,0}).data();
    writeDatagram(startReq, pa, pp);
    setSocketState(SocketState::BoundState);
}

void SocketRDT::stopStreaming()
{
    const QByteArray stopReq = RDTRequest2QNetworkDatagram(RDTRequest{0x1234,0x0000,0}).data();
    writeDatagram(stopReq, peerAddress(), peerPort());
    emit streamReset();
}

void SocketRDT::onReadyRead()
{
    do {
        QNetworkDatagram dg = receiveDatagram(pendingDatagramSize());
        QVariantList r = QNetworkDatagram2RDTResponse(dg);

        uint32_t rdt_sequence = r[0].toUInt();

        if (!m_haveBase) { // if the first read
            m_baseSeq = rdt_sequence;
            m_haveBase = true;
            m_emitTimer.restart();
            m_readings.clear();
        }

        const double t = double(quint32(rdt_sequence - m_baseSeq)) * DT;
        r.push_back(t);

        m_readings.push_back(r);
        if (m_emitTimer.elapsed() >= m_emitIntervalMs && !m_readings.isEmpty()) {
            emit bufferReady(std::exchange(m_readings, {}));
            m_emitTimer.restart();
        }

    } while(hasPendingDatagrams());
}
