#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // engine.addImportPath("qrc:/qt/qml/qdeltaplc_qml_module");
    // engine.addImportPath("qrc/qt/qml/qdeltaplc_qml_module/qml/Modules/");

    engine.loadFromModule("qdeltaplc_qml_module", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
