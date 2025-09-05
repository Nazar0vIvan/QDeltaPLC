#include "logger.h"

Logger::Logger(QObject *parent) : QObject(parent) {}

void Logger::push(const LoggerMessage& msg)
{
    const QMap<int,QString> type2string {
        {0, "ERROR"},
        {1, "OK"},
        {2, "Warning"}
    };

    QString text;
    text.append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] "));
    text.append(type2string[msg.type] + "! " + msg.initiator + ": " + msg.text);

    emit logAdded({{"type", msg.type},{"text", text}});
}

