add_subdirectory(Modules/Styles)

## QML ##
set(QML_FILES
  Main.qml
)

set(QML_FILES_STYLES
  Modules/Styles/Styles.qml
)

# set(QML_FILES_COMPONENTS
#   Modules/Components/Led.qml
#   Modules/Components/CustomGroupBox.qml
#   Modules/Components/FormComponent.qml
# )

# set(QML_FILES_QMLDIRS
#   Modules/Styles/qmldir
#   Modules/Components/qmldir
# )

qt_add_executable(qdeltaplc
    main.cpp
)

qt_add_qml_module(qdeltaplc
    URI qdeltaplc_qml_module
    VERSION 1.0
    QML_FILES
      ${QML_FILES}
)

target_link_libraries(qdeltaplc
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Gui
    PRIVATE Stylesplugin
)

include(GNUInstallDirs)

install(TARGETS qdeltaplc
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
