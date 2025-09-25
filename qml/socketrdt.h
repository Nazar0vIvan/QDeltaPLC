#ifndef SOCKETRDT_H
#define SOCKETRDT_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QDebug>
#include <QVariant>
#include <QVector>
#include <QByteArray>
#include <QtEndian>
#include <QElapsedTimer>
#include <QPointF>

#include "logger.h"

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

    Q_INVOKABLE void startStreaming(const QVariantMap& data);
    Q_INVOKABLE void stopStreaming();
    Q_INVOKABLE bool isReading = false;

signals:
    void logMessage(const LoggerMessage& msg);
    void batchReady(const QVector<QPointF>& points);
    void streamReset();

private slots:
    void slotReadData();
    void slotErrorOccurred(QAbstractSocket::SocketError socketError);
    void slotStateChanged(QAbstractSocket::SocketState state);

private:
    QNetworkDatagram RDTRequest2QNetworkDatagram(const RDTRequest& request);
    RDTResponse QNetworkDatagram2RDTResponse(const QNetworkDatagram& networkDatagram);
    QString stateToString(SocketState state);

    // batching state
    QVector<QPointF> m_readings; // Fz only
    QElapsedTimer    m_emitTimer;
    quint32          m_baseSeq = 0;
    bool             m_haveBase = false;
    int              m_emitIntervalMs = 16; // ~60 Hz GUI updates
};

#endif // SOCKETRDT_H
