add_subdirectory(Modules/Styles)
add_subdirectory(Modules/Components)

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
  Views/Dashboard/DeltaPanel/KukaAutExt.qml

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

qt_add_qml_module(qdeltaplc
  URI qdeltaplc_qml_module
  VERSION 1.0
  QML_FILES
    ${QML_FILES}
)

target_sources(qdeltaplc
  PRIVATE
    resources/pics.qrc
    resources/fonts.qrc
)

target_link_libraries(qdeltaplc
  PRIVATE
    Styles
    Components
)
