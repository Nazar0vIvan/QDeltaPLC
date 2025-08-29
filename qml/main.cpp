#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "deltaplcsocket.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module");
    engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module/qml/Modules/");

    QQmlContext* rootContext = engine.rootContext();

    DeltaPLCSocket plcSocket;
    rootContext->setContextProperty("plcSocket", &plcSocket);

    engine.loadFromModule("qdeltaplc_qml_module", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
