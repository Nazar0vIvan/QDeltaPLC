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
#include <QQmlEngine>

#include "viewport/viewportqmltypes.h"
#include "assetpaths.h"
#include "robot/robotcontroller.h"

#include "backendqmltypes.h"
#include "workspacecontroller.h"
#include "devices/devicehub.h"

#include "devices/fts/rdtmessage.h"
#include "devices/fts/ftsdevice.h"
#include "devices/plc/plcdevice.h"
#include "devices/rsi/rsidevice.h"

#include <QDebug>

namespace {

void logViewportMessage(const QString& text, bool error)
{
  qInfo().noquote() << (error ? "[ERROR]" : "[INFO]") << "3D:" << text;
}

} // namespace

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  qSetMessagePattern(QStringLiteral("[%{time yyyy-MM-dd hh:mm:ss}] %{message}"));

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
  qRegisterMetaType<QVector<V6d>>("QVector<V6d>");

  auto* plcDev = new PlcDevice(QStringLiteral("PLC_AS332T"));
  auto* ftsDev = new FtsDevice(QStringLiteral("FTS_Delta"));
  auto* rsiDev = new RsiDevice(QStringLiteral("KRC4_RSI"));

  DeviceHub hub;

  hub.add(QStringLiteral("plc"), plcDev, DeviceHub::DeviceGroup::General);
  hub.add(QStringLiteral("fts"), ftsDev, DeviceHub::DeviceGroup::Control);
  hub.add(QStringLiteral("rsi"), rsiDev, DeviceHub::DeviceGroup::Control);

  DeviceHubQml::s_inst = &hub;

  hub.startAll();

  // QmlChartBridge chartBridge;
  // QObject::connect(SocketFTS, &SocketFTS::bufferReady, &chartBridge, &QmlChartBridge::onBatch, Qt::QueuedConnection);
  // QObject::connect(SocketFTS, &SocketFTS::streamReset, &chartBridge, &QmlChartBridge::reset, Qt::QueuedConnection);

  // The scene outlives the QML engine and survives workspace recreation.
  SceneModel scene;
  SceneSingletonQml::s_inst = &scene;
  SceneEndEffectors endEffectors;
  SceneEndEffectorsQml::s_inst = &endEffectors;

  SceneMachining machining(&scene, &endEffectors);
  SceneMachiningQml::s_inst = &machining;

  SceneMachiningPreparation preparation(&scene, &machining);
  SceneMachiningPreparationQml::s_inst = &preparation;
  // Implicitly shared corrections and their log identifier cross to ControlIO together.
  QObject::connect(&preparation, &SceneMachiningPreparation::correctionsReady,
                   rsiDev, &RsiDevice::setCorrections, Qt::QueuedConnection);
  // Disable result dispatch and finish the worker before shutting down ControlIO.
  QObject::connect(&app, &QApplication::aboutToQuit, &preparation, &SceneMachiningPreparation::shutdown);
  QObject::connect(&app, &QApplication::aboutToQuit, &hub, &DeviceHub::stopAll);

  RoboCrap3D::OccController viewportController(
      RoboCrap3D::ViewportAssets::applicationAssets().occtResources);
  scene.setViewport(&viewportController);
  QObject::connect(&machining, &SceneMachining::draftAxisChanged,
                   &viewportController, &RoboCrap3D::OccController::setDraftAxis);
  OccControllerQml::s_inst = &viewportController;
  RoboCrap3D::RobotController robotController(&viewportController, &endEffectors, &machining);
  machining.setRobotController(&robotController);
  WorkspaceController workspaceController(&scene, &machining, &endEffectors);
  QObject::connect(&workspaceController, &WorkspaceController::selectionChanged,
                   &viewportController, &RoboCrap3D::OccController::setSelectedObjects);
  QObject::connect(&viewportController, &RoboCrap3D::OccController::applicationSelectionRequested,
                   &workspaceController, &WorkspaceController::selectObject);
  QObject::connect(&viewportController, &RoboCrap3D::OccController::deleteSelectionRequested,
                   &workspaceController, &WorkspaceController::deleteSelected);

  QObject::connect(&viewportController, &RoboCrap3D::OccController::message, &app, &logViewportMessage);
  QObject::connect(&robotController, &RoboCrap3D::RobotController::message, &app, &logViewportMessage);

  robotController.loadRobot();

  QQmlApplicationEngine engine;

#ifdef USE_FELGO_HOT_RELOAD
  felgo.initialize(&engine);
#endif

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

#endif

  return app.exec();
}
