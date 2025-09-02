#ifndef DELTAPLCSOCKET_H
#define DELTAPLCSOCKET_H

#include <QTcpSocket>
#include <QThread>

#include "logger.h"

#define LOCAL_ADDRESS "192.168.2.1"
#define LOCAL_PORT 2222
#define PEER_ADDRESS "192.168.2.5"
#define PEER_PORT 3333

class DeltaPLCSocket : public QTcpSocket
{
    Q_OBJECT
public:
    DeltaPLCSocket(const QString& name, QObject *parent = nullptr);
    ~DeltaPLCSocket();

signals:
    void socketConfigChanged(const MessageDescriptor& desc);
    void errorOccurredMessage(const MessageDescriptor& desc);

public slots:
    void slotSocketDataChanged(const QVariantMap& data);

private:
    bool tearDownToUnconnected(int ms = 300);
    QString hostName;
};

#endif // DELTAPLCSOCKET_H
