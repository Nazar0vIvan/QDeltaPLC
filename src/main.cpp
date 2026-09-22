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
#include "3d/viewportqmltypes.h"

#include "network/backendqmltypes.h"
#include "network/devicehub.h"

#include "network/fts/rdtmessage.h"
#include "network/fts/ftsdevice.h"
#include "network/plc/plcdevice.h"
#include "network/rsi/rsidevice.h"

#include <QWindow>
#include <QDebug>

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

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

  DeviceHubQml::s_inst = &hub;

  QObject::connect(ftsDev, &FtsDevice::dataSampleHFReady, rsiDev, &RsiDevice::setForce);

  hub.startAll();

  QObject::connect(&app, &QApplication::aboutToQuit, &hub, &DeviceHub::stopAll);

  // QmlChartBridge chartBridge;
  // QObject::connect(SocketFTS, &SocketFTS::bufferReady, &chartBridge, &QmlChartBridge::onBatch, Qt::QueuedConnection);
  // QObject::connect(SocketFTS, &SocketFTS::streamReset, &chartBridge, &QmlChartBridge::reset, Qt::QueuedConnection);

  // The scene outlives the QML engine and survives workspace recreation.
  ApplicationScene scene;
  ApplicationSceneQml::s_inst = &scene;
  // Preserve the existing metadata-only sample rows until their producers exist.
  scene.addObject(QStringLiteral("rough-plane"), QStringLiteral("Plane P1"), SceneObject::Plane, SceneObject::Rough);
  scene.addObject(QStringLiteral("rough-cylinder"), QStringLiteral("Cylinder C1"), SceneObject::Cylinder, SceneObject::Rough);
  scene.addObject(QStringLiteral("rough-cone"), QStringLiteral("Cone K1"), SceneObject::Cone, SceneObject::Rough);
  scene.addObject(QStringLiteral("precise-plane"), QStringLiteral("Plane P1"), SceneObject::Plane, SceneObject::Precise);
  scene.addObject(QStringLiteral("precise-cylinder"), QStringLiteral("Cylinder C1"), SceneObject::Cylinder, SceneObject::Precise);
  scene.addObject(QStringLiteral("edge-1"), QStringLiteral("Edge E1"), SceneObject::Edge, SceneObject::Unclassified);
  scene.addObject(QStringLiteral("edge-2"), QStringLiteral("Edge E2"), SceneObject::Edge, SceneObject::Unclassified);
  scene.addObject(QStringLiteral("scan-1"), QStringLiteral("Scan S1"), SceneObject::ScanPath, SceneObject::Unclassified);
  scene.addObject(QStringLiteral("path-1"), QStringLiteral("Path P1"), SceneObject::MachiningPath, SceneObject::Unclassified);

  RoboCrap3D::OccController viewportController;
  viewportController.setApplicationScene(&scene);
  OccControllerQml::s_inst = &viewportController;

	QObject::connect(&viewportController, &RoboCrap3D::OccController::message, Logger::instance(), [](const QString& text, bool error) {
    Logger::instance()->push({text.toHtmlEscaped(), error ? 0 : 2, QStringLiteral("3D")});
  });

  QQmlApplicationEngine engine;

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

  engine.loadFromModule("robocrap_qml_module", "Main");

  if (engine.rootObjects().isEmpty())
    return 77;

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
