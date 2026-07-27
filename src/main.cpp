#include <QApplication>
#include <QFontDatabase>
#include <QMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QUrl>

#ifdef USE_FELGO_HOT_RELOAD          // ADDED: defined by felgohotreload_configure_executable()
#include <FelgoHotReload>
#include <QStandardPaths>
#include <QTimer>
#endif

#include "logger.h"

#include "network/fts/rdtmessage.h"
#include "network/fts/socketfts.h"
#include "network/plc/plcmessagemanager.h"
#include "network/plc/socketdeltaplc.h"
#include "network/rsi/socketrsi.h"

#include "network/runner/ftsrunner.h"
#include "network/runner/plcrunner.h"
#include "network/runner/rsirunner.h"

int main(int argc, char* argv[])
{
	QCoreApplication::setOrganizationName(QStringLiteral("QDelta"));
	QCoreApplication::setApplicationName(QStringLiteral("QDeltaPLC"));

  QApplication app(argc, argv);

  QFontDatabase::addApplicationFont("://fonts/roboto/Roboto-Regular.ttf");
  QFontDatabase::addApplicationFont("://fonts/roboto/Roboto-Medium.ttf");

  const int idfont = QFontDatabase::addApplicationFont("://fonts/roboto/Roboto-Bold.ttf");
  if (idfont == -1)
    qWarning() << "Failed to load font from resources!";

  qRegisterMetaType<RDTResponse>("RDTResponse");
  qRegisterMetaType<QVector<RDTResponse>>("QVector<RDTResponse>");

  auto* socketDeltaPLC = new SocketDeltaPLC(QStringLiteral("PLC_AS332T"));
  PlcRunner plcRunner(socketDeltaPLC);
  plcRunner.start();

  auto* socketFTS = new SocketFTS(QStringLiteral("FTS_Delta"));
  FtsRunner ftsRunner(socketFTS);
  ftsRunner.start();

  auto* socketRSI = new SocketRSI(QStringLiteral("KRC4_RSI"));
  RsiRunner rsiRunner(socketRSI);
  rsiRunner.start();

	qmlRegisterUncreatableType<PlcMessageManager>(
		"qdeltaplc_qml_module", 1, 0,
		"PlcMessage", "PlcMessage is not creatable from QML"
	);

  // QmlChartBridge chartBridge;
  // QObject::connect(SocketFTS, &SocketFTS::bufferReady, &chartBridge, &QmlChartBridge::onBatch, Qt::QueuedConnection);
  // QObject::connect(SocketFTS, &SocketFTS::streamReset, &chartBridge, &QmlChartBridge::reset, Qt::QueuedConnection);

  QQmlApplicationEngine engine;
  engine.addImportPath(QStringLiteral(":/"));

#ifndef USE_FELGO_HOT_RELOAD
	// CHANGED: was unconditional. In hot reload builds this MUST be gone.
	// Otherwise "import Styles"/"import Components" resolve to the source tree,
	// while Felgo hot-patches its cached copies -> edits silently do nothing.
	engine.addImportPath(QStringLiteral(QDELTA_QML_MODULES_DIR));
#endif

  QQmlContext* ctx = engine.rootContext();
  ctx->setContextProperty("logger", Logger::instance());
  ctx->setContextProperty("plcRunner", &plcRunner);
  ctx->setContextProperty("ftsRunner", &ftsRunner);
  ctx->setContextProperty("rsiRunner", &rsiRunner);
  // ctx->setContextProperty("chartBridge", &chartBridge);

#ifdef USE_FELGO_HOT_RELOAD
	static FelgoHotReload felgoHotReload(&engine);

	qInfo() << "[FHR] app data root :" << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	qInfo() << "[FHR] qml disk cache:" << QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

	QTimer::singleShot(5000, &engine, [&engine] {
		qInfo() << "[FHR] import path list:";
		for (const QString& p : engine.importPathList())
			qInfo() << "        " << p;
	});
#else
	engine.loadFromModule("qdeltaplc_qml_module", "Main");
	if (engine.rootObjects().isEmpty())
		return EXIT_FAILURE;
#endif

  return app.exec();
}
