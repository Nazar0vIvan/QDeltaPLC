#include "logger.h"

Logger::Logger(QObject *parent) : QObject(parent) {}

void Logger::push(const MessageDescriptor& desc)
{
    QString text;
    text.append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] "));
    text.append(desc.initiator + ": " + desc.text);
    emit logAdded({{"type", desc.type},{"text", text}});
}

