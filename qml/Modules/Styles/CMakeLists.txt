cmake_minimum_required(VERSION 3.16)

set_source_files_properties(Styles.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(Styles STATIC)

qt_add_qml_module(Styles
    URI Styles
    VERSION 1.0
    NO_PLUGIN
    QML_FILES
        Styles.qml
)

target_link_libraries(Styles PRIVATE Qt6::Qml)
