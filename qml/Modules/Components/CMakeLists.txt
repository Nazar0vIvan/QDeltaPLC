cmake_minimum_required(VERSION 3.16)

qt_add_library(Components STATIC)

qt_add_qml_module(Components
    URI Components
    VERSION 1.0
    QML_FILES
        QxLed.qml
        QxGroupBox.qml
        QxButton.qml
        QxField.qml
        QxSwitch.qml
        QxTextField.qml
)

target_link_libraries(Components PRIVATE Qt6::Qml)
