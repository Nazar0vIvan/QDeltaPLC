#include "logger.h"

Logger::Logger(QObject *parent) : QObject(parent) {}

void Logger::push(const QString &message)
{
    emit logAdded(message);
}

