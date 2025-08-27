add_subdirectory(Modules/Styles)
add_subdirectory(Modules/Components)

## QML ##
set(QML_FILES
  Main.qml
  Logger.qml
)

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
    PRIVATE Componentsplugin
)

include(GNUInstallDirs)

install(TARGETS qdeltaplc
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
