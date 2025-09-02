#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QDate>
#include <QDebug>

struct MessageDescriptor {
    QString text = "";
    int type = 0;
    QString initiator = "";
};

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
    void logAdded(const QVariantMap& message);

public slots:
    void push(const MessageDescriptor& desc);

};

#endif // LOGGER_H
