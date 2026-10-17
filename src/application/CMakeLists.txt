qt_add_executable(robocrap
  assetpaths.cpp
  assetpaths.h
  backendqmltypes.h
  devices/devicehub.cpp
  devices/devicehub.h
  devices/deviceprofilemodel.cpp
  devices/deviceprofilemodel.h
  devices/devicerunner.cpp
  devices/devicerunner.h
  main.cpp
  robot/robotcontroller.cpp
  robot/robotcontroller.h
  robot/robotdefinition.cpp
  robot/robotdefinition.h
  scene/sceneendeffectors.cpp
  scene/sceneendeffectors.h
  scene/scenegeometry.h
  scene/scenemachining.cpp
  scene/scenemachining.h
  scene/scenemodel.cpp
  scene/scenemodel.h
  scene/sceneobject.cpp
  scene/sceneobject.h
  scene/scenesurfaceimporter.cpp
  scene/scenesurfaceimporter.h
  workspacecontroller.cpp
  workspacecontroller.h
  ../scene/scenemachiningpreparation.h
  ../scene/scenemachiningpreparation.cpp
  ../scene/chamfershareddata.h
  ../scene/chamfershareddata.cpp
  qmlchartbridge.h
  qmlchartbridge.cpp
)
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set_target_properties(robocrap PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

if(USE_HOT_RELOAD)
  qt_add_resources(robocrap "felgo_config"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES "qml/config.json"
  )
  felgo_configure_executable(robocrap)
  felgohotreload_configure_executable(robocrap)
endif()

if(USE_HOT_RELOAD)
  qt_add_qml_module(robocrap
    URI robocrap_qml_module
    VERSION 1.0
    ${QML_OPTIONS}
  )
else()
  qt_add_qml_module(robocrap
    URI robocrap_qml_module
    VERSION 1.0
    ${QML_OPTIONS}
    IMPORTS
      Styles
      Components
      RoboCrap.Viewport3D
    QML_FILES
      "qml/Main.qml"
      "qml/MenuBar/MainMenuBar.qml"
      "qml/Views/Dashboard/DeltaPanel/DeltaModuleAP.qml"
      "qml/Views/Dashboard/DeltaPanel/DeltaPanel.qml"
      "qml/Views/Dashboard/DeltaPanel/LedsPanel.qml"
      "qml/Views/Dashboard/DeltaPanel/DeltaModuleInput.qml"
      "qml/Views/Dashboard/DeltaPanel/DeltaModuleOutput.qml"
      "qml/Views/Dashboard/FtsPanel/FtsPanel.qml"
      "qml/Views/Dashboard/FtsPanel/FtsBars.qml"
      "qml/Views/Dashboard/KukaPanel/KukaPanel.qml"
      "qml/Views/Network/Network.qml"
      "qml/Views/Network/NetworkPanel.qml"
      "qml/Views/Network/ConnectionsTable.qml"
      "qml/Views/Viewport3D/RobotViewport.qml"
      "qml/Views/Workspace/WorkflowPanel.qml"
      "qml/Views/Workspace/ModeToolBar.qml"
      "qml/Views/Workspace/ScenePanel.qml"
      "qml/Views/Workspace/PropertiesPanel.qml"
      "qml/Views/Workspace/EndEffectorPanel.qml"
      "qml/Views/Workspace/MachiningPanel.qml"
  )
endif()

target_sources(robocrap PRIVATE
  "${PROJECT_SOURCE_DIR}/resources/pics.qrc"
  "${PROJECT_SOURCE_DIR}/resources/fonts.qrc"
  "${PROJECT_SOURCE_DIR}/resources/json.qrc"
)
target_include_directories(robocrap PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/scene
  ${CMAKE_CURRENT_SOURCE_DIR}/devices
  ${CMAKE_CURRENT_SOURCE_DIR}/robot
  ${CMAKE_CURRENT_BINARY_DIR}
  # Original header names used by the foreign-type registrar.
  ${PROJECT_SOURCE_DIR}/src/devices/include/devices/fts
  ${PROJECT_SOURCE_DIR}/src/devices/include/devices/plc
  # Frozen RSI preparation/coordination headers and their generated moc consumers.
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/src/scene
)
target_link_libraries(robocrap PRIVATE
  RoboCrapGeometry
  RoboCrapKinematics
  RoboCrapTrajectories
  RoboCrapDevices
  RoboCrap3D
  RoboCrap3Dplugin
  Qt6::Core
  Qt6::Gui
  Qt6::Quick
  Qt6::Qml
  Qt6::Network
  Qt6::Charts
  Qt6::Widgets
  Styles
  Components
)

if(USE_HOT_RELOAD)
  target_link_libraries(robocrap PRIVATE Felgo)
endif()
if(NOT USE_HOT_RELOAD)
  qt_import_qml_plugins(robocrap)
endif()

# POST_BUILD must be attached in the directory that declares the executable.
robocrap_deploy_occt(robocrap "${ROBOCRAP_OCCT_PREFIX}")
