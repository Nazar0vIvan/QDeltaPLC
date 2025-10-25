#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>

#include "socketrunner.h"
#include "socketdeltaplc.h"
#include "socketrdt.h"
#include "socketrsi.h"
// #include "qmlchartbridge.h"
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
  QObject::connect(socketDeltaPLC, &SocketDeltaPLC::plcDataReady, &plcRunner, &TcpSocketRunner::plcDataReady, Qt::QueuedConnection);
  plcRunner.start();

  SocketRDT* socketRDT = new SocketRDT(QStringLiteral("FTS_Delta"));
  UdpSocketRunner ftsRunner(socketRDT);
  QObject::connect(socketRDT, &SocketRDT::bufferReady, &ftsRunner, &UdpSocketRunner::onBufferReady, Qt::QueuedConnection);
  ftsRunner.start();

  SocketRSI* socketRSI = new SocketRSI(QStringLiteral("KRC4_RSI"));
  UdpSocketRunner rsiRunner(socketRSI);
  rsiRunner.start();

  QObject::connect(&app, &QApplication::aboutToQuit, &plcRunner, &AbstractSocketRunner::stop);
  QObject::connect(&app, &QApplication::aboutToQuit, &ftsRunner, &AbstractSocketRunner::stop);
  QObject::connect(&app, &QApplication::aboutToQuit, &rsiRunner, &AbstractSocketRunner::stop);

  // QmlChartBridge chartBridge;
  // QObject::connect(socketRDT, &SocketRDT::bufferReady, &chartBridge, &QmlChartBridge::onBatch, Qt::QueuedConnection);
  // QObject::connect(socketRDT, &SocketRDT::streamReset, &chartBridge, &QmlChartBridge::reset, Qt::QueuedConnection);

  QQmlApplicationEngine engine;
  // engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module");
  engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module/qml/Modules/");

  QQmlContext* ctx  = engine.rootContext();
  ctx->setContextProperty("logger", Logger::instance());
  ctx->setContextProperty("plcRunner", &plcRunner);
  ctx->setContextProperty("ftsRunner", &ftsRunner);
  ctx->setContextProperty("rsiRunner", &rsiRunner);
  // ctx->setContextProperty("chartBridge", &chartBridge);

  engine.loadFromModule("qdeltaplc_qml_module", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;

  return app.exec();
}
