#pragma once

#include <QMetaType>
#include <QString>

struct LoggerMessage {
	QString text = "";
	int type = 0;
	QString initiator = "";
};

Q_DECLARE_METATYPE(LoggerMessage)
