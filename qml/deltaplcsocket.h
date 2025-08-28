#ifndef DELTAPLCSOCKET_H
#define DELTAPLCSOCKET_H

#include <QTcpSocket>

class DeltaPLCSocket : public QTcpSocket
{
    Q_OBJECT
public:
    DeltaPLCSocket();
};

#endif // DELTAPLCSOCKET_H
