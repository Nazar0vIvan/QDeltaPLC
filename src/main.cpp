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

#include "network/backendqmltypes.h"
#include "network/devicehub.h"

#include "network/fts/rdtmessage.h"
#include "network/fts/ftsdevice.h"
#include "network/plc/plcdevice.h"
#include "network/rsi/rsidevice.h"

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
  qRegisterMetaType<LoggerMessage>("LoggerMessage");

  auto* plcDev = new PlcDevice(QStringLiteral("PLC_AS332T"));
  auto* ftsDev = new FtsDevice(QStringLiteral("FTS_Delta"));
  auto* rsiDev = new RsiDevice(QStringLiteral("KRC4_RSI"));

  DeviceHub hub;

  hub.add(QStringLiteral("plc"), plcDev, DeviceHub::DeviceGroup::General);
  hub.add(QStringLiteral("fts"), ftsDev, DeviceHub::DeviceGroup::Control);
  hub.add(QStringLiteral("rsi"), rsiDev, DeviceHub::DeviceGroup::Control);

  DeviceHubQml::s_inst = &hub;

  QObject::connect(ftsDev, &FtsDevice::dataSampleHFReady, rsiDev, &RsiDevice::setForce);

  hub.startAll();

  QObject::connect(&app, &QApplication::aboutToQuit, &hub, &DeviceHub::stopAll);

  // QmlChartBridge chartBridge;
  // QObject::connect(SocketFTS, &SocketFTS::bufferReady, &chartBridge, &QmlChartBridge::onBatch, Qt::QueuedConnection);
  // QObject::connect(SocketFTS, &SocketFTS::streamReset, &chartBridge, &QmlChartBridge::reset, Qt::QueuedConnection);

  QQmlApplicationEngine engine;
  felgo.initialize(&engine);

  QQmlContext* ctx = engine.rootContext();
  ctx->setContextProperty("logger", Logger::instance());

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
