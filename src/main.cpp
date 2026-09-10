#ifdef USE_FELGO_HOT_RELOAD
#include <FelgoApplication>
#include <FelgoHotReload>

#include <QDir>
#include <QStandardPaths>
#endif

#include <QApplication>

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

#include <QWindow>
#include <QDebug>
#include <QFile>

int main(int argc, char* argv[])
{
  QFile trace("qdeltaplc-startup.txt");
  trace.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);

  auto mark = [&trace](const char* text) {
    trace.write(text);
    trace.write("\n");
    trace.flush();
  };

  QApplication app(argc, argv);

  mark("1 QApplication created");

#ifdef USE_FELGO_HOT_RELOAD
  FelgoApplication felgo;
#endif

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

  mark("2 devices created");

  DeviceHubQml::s_inst = &hub;

  QObject::connect(ftsDev, &FtsDevice::dataSampleHFReady, rsiDev, &RsiDevice::setForce);

  hub.startAll();

  mark("3 hub started");

  QObject::connect(&app, &QApplication::aboutToQuit, &hub, &DeviceHub::stopAll);

  // QmlChartBridge chartBridge;
  // QObject::connect(SocketFTS, &SocketFTS::bufferReady, &chartBridge, &QmlChartBridge::onBatch, Qt::QueuedConnection);
  // QObject::connect(SocketFTS, &SocketFTS::streamReset, &chartBridge, &QmlChartBridge::reset, Qt::QueuedConnection);

  QQmlApplicationEngine engine;

  mark("4 engine created");

#ifdef USE_FELGO_HOT_RELOAD
  felgo.initialize(&engine);
#endif

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

  mark("5 before loadFromModule");

  engine.loadFromModule("qdeltaplc_qml_module", "Main");

  mark("6 after loadFromModule");

  mark("7 root checked");

  if (engine.rootObjects().isEmpty())
    return 77;

  mark("8 before app.exec");

  qDebug() << "Root objects:" << engine.rootObjects().size();
  qDebug() << "Windows:" << QGuiApplication::allWindows().size();

  for (QWindow* window : QGuiApplication::allWindows()) {
    qDebug() << "Window:"
             << window
             << "visible:" << window->isVisible()
             << "geometry:" << window->geometry();
  }

  if (engine.rootObjects().isEmpty()) return -1;

#endif

  return app.exec();
}
