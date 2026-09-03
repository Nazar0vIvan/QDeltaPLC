#include <QApplication>
#include <FelgoApplication>

#ifdef USE_FELGO_HOT_RELOAD
#include <FelgoHotReload>
#endif

#include <QFontDatabase>
#include <QMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>

#include "logger.h"

#include "network/fts/rdtmessage.h"
#include "network/fts/socketfts.h"
#include "network/plc/plcmessagemanager.h"
#include "network/plc/socketdeltaplc.h"
#include "network/rsi/socketrsi.h"

#include "network/runner/abstractsocketrunner.h"
#include "network/runner/ftsrunner.h"
#include "network/runner/plcrunner.h"
#include "network/runner/rsirunner.h"
#include "network/common/deviceprofiles.h"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  FelgoApplication felgo;

  QFontDatabase::addApplicationFont("://fonts/roboto/Roboto-Regular.ttf");
  QFontDatabase::addApplicationFont("://fonts/roboto/Roboto-Medium.ttf");

  const int idfont = QFontDatabase::addApplicationFont("://fonts/roboto/Roboto-Bold.ttf");
  if (idfont == -1) {
    qWarning() << "Failed to load font from resources!";
  }

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

  QObject::connect(&app, &QApplication::aboutToQuit, &plcRunner, &AbstractSocketRunner::stop);
  QObject::connect(&app, &QApplication::aboutToQuit, &ftsRunner, &AbstractSocketRunner::stop);
  QObject::connect(&app, &QApplication::aboutToQuit, &rsiRunner, &AbstractSocketRunner::stop);
  QObject::connect(socketFTS, &SocketFTS::dataSampleHFReady, socketRSI, &SocketRSI::setForce);

  // QmlChartBridge chartBridge;
  // QObject::connect(SocketFTS, &SocketFTS::bufferReady, &chartBridge, &QmlChartBridge::onBatch, Qt::QueuedConnection);
  // QObject::connect(SocketFTS, &SocketFTS::streamReset, &chartBridge, &QmlChartBridge::reset, Qt::QueuedConnection);

  QQmlApplicationEngine engine;
  felgo.initialize(&engine);

  QQmlContext* ctx = engine.rootContext();
  ctx->setContextProperty("logger", Logger::instance());
  ctx->setContextProperty("plcRunner", &plcRunner);
  ctx->setContextProperty("ftsRunner", &ftsRunner);
  ctx->setContextProperty("rsiRunner", &rsiRunner);
  // ctx->setContextProperty("chartBridge", &chartBridge);

  qmlRegisterType<DeviceProfiles>(
    "qdeltaplc_qml_module",
    1, 0,
    "DeviceProfiles"
  );

  qmlRegisterUncreatableType<PlcMessageManager>(
    "qdeltaplc_qml_module",
    1, 0,
    "PlcMessage",
    "PlcMessage is not creatable from QML"
  );

  // engine.loadFromModule("qdeltaplc_qml_module", "Main");
  // const QUrl mainQmlUrl = QUrl::fromLocalFile(QStringLiteral(QDELTA_QML_SOURCE_DIR "/Main.qml"));
  // engine.load(mainQmlUrl);

#ifdef USE_FELGO_HOT_RELOAD

  const QString appName = QCoreApplication::applicationName();
  const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

  const QString hotReloadModulesPath =
      QDir(appDataPath).filePath(
        QStringLiteral("FelgoDevApp/%1/%1/qml/Modules").arg(appName)
      );

  engine.addImportPath(hotReloadModulesPath);

  qWarning() << "Application name:"
             << QCoreApplication::applicationName();

  qWarning() << "App data:"
             << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

  FelgoHotReload felgoHotReload(&engine);

#else

  const QUrl mainQmlUrl = QUrl::fromLocalFile(QStringLiteral(QDELTA_QML_SOURCE_DIR "/Main.qml"));
  engine.load(mainQmlUrl);

  if (engine.rootObjects().isEmpty())
    return -1;

#endif

  return app.exec();
}
