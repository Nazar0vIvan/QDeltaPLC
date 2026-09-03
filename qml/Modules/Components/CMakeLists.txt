qt_add_library(Components STATIC)

qt_add_qml_module(Components
  URI Components
  VERSION 1.0
  ${QDELTA_QML_OPTIONS}
  OUTPUT_DIRECTORY "${QT_QML_OUTPUT_DIRECTORY}/Components"
  IMPORTS
    Styles
  NO_PLUGIN
  QML_FILES
    QxButton.qml
    QxComboBox.qml
    QxGroupBox.qml
    QxHField.qml
    QxLed.qml
    QxProgressBar.qml
    QxStepButton.qml
    QxSwitch.qml
    QxTextInput.qml
    QxToolButton.qml
    QxUploadFile.qml
    QxVField.qml
)

target_link_libraries(Components
  PRIVATE
    Qt6::Qml
    Styles
)
