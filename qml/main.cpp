#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "socketrunner.h"
#include "socketdeltaplc.h"
#include "socketrdt.h"
#include "qmlchartbridge.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    TcpSocketRunner plcRunner(new SocketDeltaPLC(QStringLiteral("PLC_AS332T")));
    plcRunner.start();

    SocketRDT* socketRDT = new SocketRDT(QStringLiteral("FTS_Delta"));
    UdpSocketRunner ftsRunner(socketRDT);
    QObject::connect(socketRDT, &SocketRDT::bufferReady, &ftsRunner, &UdpSocketRunner::onBufferReady, Qt::QueuedConnection);
    ftsRunner.start();

    QObject::connect(&app, &QApplication::aboutToQuit, &plcRunner, &AbstractSocketRunner::stop);
    QObject::connect(&app, &QApplication::aboutToQuit, &ftsRunner, &AbstractSocketRunner::stop);

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
    // ctx->setContextProperty("chartBridge", &chartBridge);

    engine.loadFromModule("qdeltaplc_qml_module", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
