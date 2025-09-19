#include "socketrdt.h"

SocketRDT::SocketRDT(const QString& name, QObject* parent) : QUdpSocket(parent)
{
    // setID(2);
    // setProtocolName("RDT");

    this->setObjectName(name);

    setLocalPort(RDT_LOCAL_PORT);
    setPeerPort(RDT_PEER_PORT);

    setOpenMode(QIODeviceBase::ReadWrite);

    // parmsModel()->setID("rdt");
    // parmsModel()->appendParameter("rdt_seq", "int",    "ct");
    // parmsModel()->appendParameter("ft_seq",  "int",    "ct");
    // parmsModel()->appendParameter("status",  "int",    "ct");
    // parmsModel()->appendParameter("Fx",      "double", "N",  -1980.0, 1980.0);
    // parmsModel()->appendParameter("Fy",      "double", "N",  -660.0,  660.0);
    // parmsModel()->appendParameter("Fz",      "double", "N",  -660.0,  660.0);
    // parmsModel()->appendParameter("Tx",      "double", "Nm", -60.0,    60.0);
    // parmsModel()->appendParameter("Ty",      "double", "Nm", -60.0,    60.0);
    // parmsModel()->appendParameter("Tz",      "double", "Nm", -60.0,    60.0);

    connect(this, &SocketRDT::readyRead, this, &SocketRDT::slotReadData);
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

RDTResponse SocketRDT::QNetworkDatagram2RDTResponse(const QNetworkDatagram& networkDatagram)
{
    QByteArray bytes(networkDatagram.data());

    uint32_t rdt_sequence = qFromBigEndian<uint32_t>(bytes.left(4).data());
    uint32_t ft_sequence = qFromBigEndian<uint32_t>(bytes.right(32).left(4).data());
    uint32_t status = qFromBigEndian<uint32_t>(bytes.right(28).left(4).data());
    int32_t Fx = qFromBigEndian<int32_t>(bytes.right(24).left(4).data());
    int32_t Fy = qFromBigEndian<int32_t>(bytes.right(20).left(4).data());
    int32_t Fz = qFromBigEndian<int32_t>(bytes.right(16).left(4).data());
    int32_t Tx = qFromBigEndian<int32_t>(bytes.right(12).left(4).data());
    int32_t Ty = qFromBigEndian<int32_t>(bytes.right(8).left(4).data());
    int32_t Tz = qFromBigEndian<int32_t>(bytes.right(4).data());

    return { rdt_sequence, ft_sequence,status, Fx,Fy,Fz,Tx,Ty,Tz };
}

void SocketRDT::startStreaming(const QVariantMap& data)
{
    QHostAddress pa = QHostAddress(data.value("peerAddress").toString());
    qint16 pp = data.value("peerPort").toUInt();

    setPeerAddress(pa); setPeerPort(pp);

    auto startRequest = RDTRequest2QNetworkDatagram(RDTRequest{0x1234,0x0002,0}).data();
    writeDatagram(startRequest, pa, pp);
}

void SocketRDT::stopStreaming()
{
    auto stopRequest = RDTRequest2QNetworkDatagram(RDTRequest{0x1234,0x0000,0}).data();
    writeDatagram(stopRequest, peerAddress(), peerPort());
}

void SocketRDT::slotReadData()
{
    qDebug() << "rdt ready read";

    //  do{
    //    QNetworkDatagram FTNetResponseDatagram = receiveDatagram(pendingDatagramSize());
    //    uint32_t rdt_sequence = qFromBigEndian<uint32_t>(FTNetResponseDatagram.data().left(4).data());
    //    uint32_t ft_sequence = qFromBigEndian<uint32_t>(FTNetResponseDatagram.data().right(32).left(4).data());
    //    uint32_t status = qFromBigEndian<uint32_t>( FTNetResponseDatagram.data().right(28).left(4).data());
    //    int32_t Fx = qFromBigEndian<int32_t>(FTNetResponseDatagram.data().right(24).left(4).data());
    //    int32_t Fy = qFromBigEndian<int32_t>(FTNetResponseDatagram.data().right(20).left(4).data());
    //    int32_t Fz = qFromBigEndian<int32_t>(FTNetResponseDatagram.data().right(16).left(4).data());
    //    int32_t Tx = qFromBigEndian<int32_t>(FTNetResponseDatagram.data().right(12).left(4).data());
    //    int32_t Ty = qFromBigEndian<int32_t>(FTNetResponseDatagram.data().right(8).left(4).data());
    //    int32_t Tz = qFromBigEndian<int32_t>(FTNetResponseDatagram.data().right(4).data());

    //    emit responceChanged(RDTResponse{ rdt_sequence, ft_sequence, status, Fx, Fy, Fz, Tx, Ty, Tz });

    //  } while(hasPendingDatagrams());
}
