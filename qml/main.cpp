#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlContext>
#include <QFontDatabase>

#include "socketrunner.h"
#include "socketdeltaplc.h"
#include "socketfts.h"
#include "socketrsi.h"
#include "logger.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  QFontDatabase::addApplicationFont("://assets/fonts/roboto/Roboto-Regular.ttf");
  QFontDatabase::addApplicationFont("://assets/fonts/roboto/Roboto-Medium.ttf");
  auto idfont = QFontDatabase::addApplicationFont("://assets/fonts/roboto/Roboto-Bold.ttf");
  if (idfont == -1) {
    qWarning() << "Failed to load font from resources!";
  }

  SocketDeltaPLC* socketDeltaPLC = new SocketDeltaPLC(QStringLiteral("PLC_AS332T"));
  TcpSocketRunner plcRunner(socketDeltaPLC);
  QObject::connect(socketDeltaPLC, &SocketDeltaPLC::plcDataReady, &plcRunner, &TcpSocketRunner::dataReady, Qt::QueuedConnection);
  plcRunner.start();

  SocketFTS* socketFTS = new SocketFTS(QStringLiteral("FTS_Delta"));
  FtsRunner ftsRunner(socketFTS);
  ftsRunner.start();

  SocketRSI* socketRSI = new SocketRSI(QStringLiteral("KRC4_RSI"));
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
  // engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module");
  engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module/qml/Modules/");

  QQmlContext* ctx  = engine.rootContext();
  ctx->setContextProperty("logger", Logger::instance());
  ctx->setContextProperty("plcRunner", &plcRunner);
  ctx->setContextProperty("ftsRunner", &ftsRunner);
  ctx->setContextProperty("rsiRunner", &rsiRunner);
  // ctx->setContextProperty("chartBridge", &chartBridge);

  qmlRegisterUncreatableType<PlcMessageManager>(
    "qdeltaplc_qml_module", // import URI
    1, 0,                   // version
    "PlcMessage",           // QML name
    "PlcMessage is not creatable from QML"
  );

  engine.loadFromModule("qdeltaplc_qml_module", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  return app.exec();
}
