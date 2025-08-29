#ifndef DELTAPLCSOCKET_H
#define DELTAPLCSOCKET_H

#include <QTcpSocket>

#define LOCAL_ADDRESS "192.168.2.1"
#define LOCAL_PORT 2222
#define PEER_ADDRESS "192.168.2.5"
#define PEER_PORT 3333

class DeltaPLCSocket : public QTcpSocket
{
    Q_OBJECT
public:
    DeltaPLCSocket(QObject *parent = nullptr);

signals:
    void socketDataChanged(const QVariantMap &data);

public slots:
    void slotSocketDataChanged(const QVariantMap &data);
};

#endif // DELTAPLCSOCKET_H
