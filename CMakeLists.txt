cmake_minimum_required(VERSION 3.16)

project(RoboCrap VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
  Core
  Gui
  Quick
  Qml
  Network
  Charts
  Xml
  Widgets
)
find_package(Qt6 REQUIRED COMPONENTS Core)

option(
  FAST_QML_BUILD
  "Disable QML cache generation for faster development builds"
  ON
)

option(
  USE_HOT_RELOAD
  "Enable Felgo Hot Reload"
  ON
)

if(USE_HOT_RELOAD)
  find_package(Felgo REQUIRED)
  find_package(FelgoHotReload CONFIG REQUIRED)

  set(PRODUCT_IDENTIFIER "com.robocrap.robocrap")
  set(PRODUCT_VERSION_NAME "0.1.0")
  set(PRODUCT_VERSION_CODE 1)
  set(PRODUCT_STAGE "test")
endif()


qt_standard_project_setup(REQUIRES 6.8)

set(QML_OPTIONS)

if(FAST_QML_BUILD)
  list(APPEND QML_OPTIONS NO_CACHEGEN)
endif()

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")
set(QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}" CACHE STRING "QML import paths for Qt Creator" FORCE)

qt_add_executable(robocrap
  src/main.cpp
)

if(USE_HOT_RELOAD)
  qt_add_resources(robocrap "felgo_config"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/qml/config.json"
  )

  felgo_configure_executable(robocrap)
  felgohotreload_configure_executable(robocrap)
endif()

add_subdirectory(src)
add_subdirectory(qml/Modules/Styles)
add_subdirectory(qml/Modules/Components)

set(QML_FILES
  qml/Main.qml
  qml/Logger.qml
  qml/MenuBar/MainMenuBar.qml
  qml/NavigationPanel/NavigationPanel.qml
  qml/NavigationPanel/NavigationMenuDelegate.qml
  qml/Views/Dashboard/DeltaPanel/DeltaModuleAP.qml
  qml/Views/Dashboard/DeltaPanel/DeltaPanel.qml
  qml/Views/Dashboard/DeltaPanel/LedsPanel.qml
  qml/Views/Dashboard/DeltaPanel/DeltaModuleInput.qml
  qml/Views/Dashboard/DeltaPanel/DeltaModuleOutput.qml
  qml/Views/Dashboard/FtsPanel/FtsPanel.qml
  qml/Views/Dashboard/FtsPanel/FtsBars.qml
  qml/Views/Dashboard/KukaPanel/KukaPanel.qml
  qml/Views/Dashboard/KukaPanel/RsiPosition.qml
  qml/Views/Dashboard/KukaPanel/RsiCoordinate.qml
  qml/Views/Dashboard/KukaPanel/Counter.qml
  qml/Views/Dashboard/Dashboard.qml
  qml/Views/Network/Network.qml
  qml/Views/Network/NetworkPanel.qml
  qml/Views/Network/ConnectionsTable.qml
  qml/Views/Viewport3D/RobotViewport.qml
  qml/Views/Viewport3D/RobotKinematicsPanel.qml
  qml/Views/Viewport3D/SixValueEditor.qml
  qml/Views/Workspace/WorkflowPanel.qml
  qml/Views/Workspace/ModeToolBar.qml
  qml/Views/Workspace/ScenePanel.qml
  qml/Views/Workspace/PropertiesPanel.qml
)

set(RESOURCE_FILES
  resources/pics.qrc
  resources/fonts.qrc
  resources/files.qrc
  resources/json.qrc
)

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
      ${QML_FILES}
  )
endif()

target_sources(robocrap PRIVATE
  ${RESOURCE_FILES}
)

target_link_libraries(robocrap PRIVATE
  RoboCrapBackend
  RoboCrapBackendplugin
  RoboCrap3D
  RoboCrap3Dplugin
  Qt6::Core
  Qt6::Gui
  Qt6::Quick
  Qt6::Qml
  Qt6::Network
  Qt6::Charts
  Qt6::Xml
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

include(GNUInstallDirs)

include(cmake/DeployOcct.cmake)
robocrap_deploy_occt(robocrap "${ROBOCRAP_OCCT_PREFIX}")

install(TARGETS robocrap
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
