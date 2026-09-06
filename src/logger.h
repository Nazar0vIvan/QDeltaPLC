#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

struct LoggerMessage {
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
	void logAdded(const QVariantMap& msg);

public slots:
	void push(const LoggerMessage& msg);
};

