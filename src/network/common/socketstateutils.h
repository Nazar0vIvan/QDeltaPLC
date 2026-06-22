#pragma once

#include <QAbstractSocket>
#include <QString>

QString socketStateName(QAbstractSocket::SocketState state);
QString socketErrorName(QAbstractSocket::SocketError error);