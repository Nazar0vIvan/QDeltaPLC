#ifndef SOCKETRDT_H
#define SOCKETRDT_H

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QDebug>
#include <QVector>
#include <QByteArray>
#include <QtEndian>
#include <QTimer>

/*
  LOCAL ADDRESS: 192.168.1.1 // can't be set
  LOCAL PORT:    59152       // must be set
  PEER ADDRESS:  192.168.1.3 // must be set
  PEER PORT:     49152       // can't be set
*/

#define RDT_LOCAL_PORT 59152
#define RDT_PEER_ADDRESS "192.168.1.3"
#define RDT_PEER_PORT 49152

#define RDT_REQUEST_LENGTH 8
#define RDT_RESPONSE_LENGTH 36

struct RDTRequest
{
    uint16_t header;
    uint16_t command;
    uint32_t sampleCount;
};

struct RDTResponse
{
    uint32_t rdt_sequence;
    uint32_t ft_sequence;
    uint32_t status;
    int32_t Fx;
    int32_t Fy;
    int32_t Fz;
    int32_t Tx;
    int32_t Ty;
    int32_t Tz;
};

class SocketRDT : public QUdpSocket
{

    Q_OBJECT

public:
    SocketRDT(const QString& name, QObject* parent = nullptr);

signals:
    void responceChanged(const RDTResponse& responce);

public slots:
    void slotStartStreaming();
    void slotStopStreaming();

private slots:
    void slotReadData();

private:
    QString hostName;

    QNetworkDatagram RDTRequest2QNetworkDatagram(const RDTRequest& request);
    RDTResponse QNetworkDatagram2RDTResponse(const QNetworkDatagram& networkDatagram);
};

#endif // SOCKETRDT_H
