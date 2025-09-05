#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "socketrunner.h"
#include "deltaplcsocket.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module");
    engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module/qml/Modules/");

    SocketRunner runner(new DeltaPLCSocket("plcSocket"), &app);
    runner.start();

    QQmlContext* ctx  = engine.rootContext();
    ctx->setContextProperty("logger", Logger::instance());
    ctx->setContextProperty("plcSocket", runner.socket());
    ctx->setContextProperty("plcRunner", &runner);

    engine.loadFromModule("qdeltaplc_qml_module", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
