#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QDebug>

// Singleton
class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger* instance() {
        static Logger inst;
        return &inst;
    }
private:
    Logger(QObject *parent = nullptr);

signals:
    void logAdded(const QString& message);

public slots:
    void push(const QString& message);

};

#endif // LOGGER_H
