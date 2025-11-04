add_subdirectory(Modules/Styles)
add_subdirectory(Modules/Components)

## QML ##
set(QML_FILES
  Main.qml
  Logger.qml
  NavigationPanel/NavigationPanel.qml
  NavigationPanel/NavigationMenuDelegate.qml
  Views/Dashboard/DeltaPanel/DeltaModuleAP.qml
  Views/Dashboard/DeltaPanel/DeltaPanel.qml
  Views/Dashboard/DeltaPanel/LedsPanel.qml
  Views/Dashboard/DeltaPanel/DeltaModuleInput.qml
  Views/Dashboard/DeltaPanel/DeltaModuleOutput.qml
  Views/Dashboard/DeltaPanel/DeltaNetwork.qml
  Views/Dashboard/FtsPanel/FtsPanel.qml
  Views/Dashboard/FtsPanel/FtsBars.qml
  Views/Dashboard/KukaPanel/KukaPanel.qml
  Views/Dashboard/KukaPanel/RsiPosition.qml
  Views/Dashboard/KukaPanel/RsiCoordinate.qml
  Views/Dashboard/KukaPanel/Counter.qml
  Views/Dashboard/Dashboard.qml
  Views/Options/Options.qml
  Views/Options/OptionsSection.qml
  Views/Test/Test.qml
)

## CPP ##
set(CPP_FILES
  logger.h
  logger.cpp
  plcmessagemanager.h
  plcmessagemanager.cpp
  qmlchartbridge.h
  qmlchartbridge.cpp
  socketdeltaplc.h
  socketdeltaplc.cpp
  socketrdt.h
  socketrdt.cpp
  socketrsi.h
  socketrsi.cpp
  socketrunner.h
  socketrunner.cpp
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
  RESOURCES
    pics.qrc
    fonts.qrc
    files.qrc
)

target_link_libraries(qdeltaplc
  PRIVATE Qt6::Quick
  PRIVATE Qt6::Gui
  PRIVATE Qt6::Network
  PRIVATE Qt6::Charts
  PRIVATE Qt6::Xml
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
