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
#include <QVariantList>
#include <QVariant>

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
    Q_GADGET
    Q_PROPERTY(uint32_t rdt_sequence MEMBER rdt_sequence)
    Q_PROPERTY(uint32_t ft_sequence MEMBER ft_sequence)
    Q_PROPERTY(uint32_t status MEMBER status)
    Q_PROPERTY(int32_t Fx MEMBER Fx)
    Q_PROPERTY(int32_t Fy MEMBER Fy)
    Q_PROPERTY(int32_t Fz MEMBER Fz)
    Q_PROPERTY(int32_t Tx MEMBER Tx)
    Q_PROPERTY(int32_t Ty MEMBER Ty)
    Q_PROPERTY(int32_t Tz MEMBER Tz)

public:
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

Q_DECLARE_METATYPE(RDTResponse)

class SocketRDT : public QUdpSocket
{
    Q_OBJECT

public:
    explicit SocketRDT(const QString& name, QObject* parent = nullptr);

    Q_INVOKABLE void startStreaming();
    Q_INVOKABLE void stopStreaming();
    Q_INVOKABLE void setSocketConfig(const QVariantMap& config);

signals:
    void logMessage(const LoggerMessage& msg);
    void bufferReady(const QVector<QVariantList>& points);
    void streamReset();

private slots:
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
    void onStateChanged(QAbstractSocket::SocketState state);

private:
    QNetworkDatagram RDTRequest2QNetworkDatagram(const RDTRequest& request);
    //RDTResponse QNetworkDatagram2RDTResponse(const QNetworkDatagram& networkDatagram);
    QVariantList QNetworkDatagram2RDTResponse(const QNetworkDatagram& networkDatagram);
    QString stateToString(SocketState state);

    // batching state
    QVector<QVariantList> m_readings;
    QElapsedTimer m_emitTimer;
    quint32       m_baseSeq = 0;
    bool          m_isFirstRead = false;
    int           m_emitIntervalMs = 16; // ~60 Hz GUI updates

    QHostAddress m_la;
    quint16 m_lp = 0;
    QHostAddress m_pa;
    quint16 m_pp = 0;
};

#endif // SOCKETRDT_H
