#include <QApplication>
#include <QFontDatabase>
#include <QMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QUrl>

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

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

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

  QQmlContext* ctx = engine.rootContext();
  ctx->setContextProperty("logger", Logger::instance());
  ctx->setContextProperty("plcRunner", &plcRunner);
  ctx->setContextProperty("ftsRunner", &ftsRunner);
  ctx->setContextProperty("rsiRunner", &rsiRunner);
  // ctx->setContextProperty("chartBridge", &chartBridge);

  qmlRegisterUncreatableType<PlcMessageManager>(
    "qdeltaplc_qml_module",
    1, 0,
    "PlcMessage",
    "PlcMessage is not creatable from QML"
  );

  engine.loadFromModule("qdeltaplc_qml_module", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  return app.exec();
}