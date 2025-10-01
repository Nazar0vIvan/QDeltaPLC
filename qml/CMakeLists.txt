add_subdirectory(Modules/Styles)
add_subdirectory(Modules/Components)

## QML ##
set(QML_FILES
  Main.qml
  Logger.qml
  PLCPanel/NetworkTcp.qml
  PLCPanel/DeltaModuleAP.qml
  PLCPanel/DeltaPanel.qml
  PLCPanel/LedsPanel.qml
  PLCPanel/DeltaModuleInput.qml
  PLCPanel/DeltaModuleOutput.qml
  FTSPanel/FTSPanel.qml
  FTSPanel/NetworkUdp.qml
  FTSPanel/FTSBars.qml
)

## CPP ##
set(CPP_FILES
  socketdeltaplc.h
  socketdeltaplc.cpp
  socketrdt.h
  socketrdt.cpp
  logger.h
  logger.cpp
  socketrunner.h
  socketrunner.cpp
  qmlchartbridge.h
  qmlchartbridge.cpp
)

qt_add_executable(qdeltaplc
    main.cpp
)

qt_add_qml_module(qdeltaplc
    URI qdeltaplc_qml_module
    VERSION 1.0
    QML_FILES
      ${QML_FILES}
    SOURCES
      ${CPP_FILES}
)

target_link_libraries(qdeltaplc
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Gui
    PRIVATE Qt6::Network
    PRIVATE Qt6::Charts
    PRIVATE Stylesplugin
    PRIVATE Componentsplugin
)

include(GNUInstallDirs)

install(TARGETS qdeltaplc
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_import_qml_plugins(qdeltaplc)
