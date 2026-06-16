qt_add_library(Styles STATIC)

set_source_files_properties(Styles.qml
  PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(Styles
  URI Styles
  VERSION 1.0
  OUTPUT_DIRECTORY "${QT_QML_OUTPUT_DIRECTORY}/Styles"
  NO_PLUGIN
  QML_FILES
    Styles.qml
)

target_link_libraries(Styles
  PRIVATE
    Qt6::Qml
)