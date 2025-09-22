#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "socketrunner.h"
#include "socketdeltaplc.h"
#include "socketrdt.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module");
    engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module/qml/Modules/");

    SocketRunner plcRunner(new SocketDeltaPLC("PLC_AS332T"), &app);
    plcRunner.start();

    SocketRunner ftsRunner(new SocketRDT("FTS_Delta"), &app);
    ftsRunner.start();

    QQmlContext* ctx  = engine.rootContext();
    ctx->setContextProperty("logger", Logger::instance());
    ctx->setContextProperty("plcRunner", &plcRunner);
    ctx->setContextProperty("ftsRunner", &ftsRunner);

    engine.loadFromModule("qdeltaplc_qml_module", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
