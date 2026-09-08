#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMetaType>

struct LoggerMessage {
	QString text = "";
	int type = 0;
	QString initiator = "";
};

Q_DECLARE_METATYPE(LoggerMessage)

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
	void logAdded(const QVariantMap& msg);

public slots:
	void push(const LoggerMessage& msg);
};

