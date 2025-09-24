#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "socketrunner.h"
#include "socketdeltaplc.h"
#include "socketrdt.h"
#include "chartbridge.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    SocketRunner plcRunner(new SocketDeltaPLC(QStringLiteral("PLC_AS332T")), &app);
    plcRunner.start();

    SocketRDT* socketRDT = new SocketRDT(QStringLiteral("FTS_Delta"));
    SocketRunner ftsRunner(socketRDT, &app);
    ftsRunner.start();

    ChartBridge chartBridge;
    QObject::connect(socketRDT, &SocketRDT::batchReady, &chartBridge, &ChartBridge::onBatch, Qt::QueuedConnection);
    QObject::connect(socketRDT, &SocketRDT::streamReset, &chartBridge, &ChartBridge::reset, Qt::QueuedConnection);

    QQmlApplicationEngine engine;
    // engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module");
    engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module/qml/Modules/");

    QQmlContext* ctx  = engine.rootContext();
    ctx->setContextProperty("logger", Logger::instance());
    ctx->setContextProperty("plcRunner", &plcRunner);
    ctx->setContextProperty("ftsRunner", &ftsRunner);
    ctx->setContextProperty("chartBridge", &chartBridge);

    engine.loadFromModule("qdeltaplc_qml_module", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
