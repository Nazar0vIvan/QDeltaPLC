qt_add_library(Components STATIC)

qt_add_qml_module(Components
  URI Components
  VERSION 1.0
  OUTPUT_DIRECTORY "${QT_QML_OUTPUT_DIRECTORY}/Components"
  IMPORTS
    Styles
  NO_PLUGIN
  QML_FILES
    QxLed.qml
    QxGroupBox.qml
    QxButton.qml
    QxField.qml
    QxSwitch.qml
    QxTextInput.qml
    QxProgressBar.qml
    QxUploadFile.qml
    QxToolButton.qml
    QxStepButton.qml
)

target_link_libraries(Components
  PRIVATE
    Qt6::Qml
    Styles
)