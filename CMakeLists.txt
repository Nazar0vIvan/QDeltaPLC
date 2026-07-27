cmake_minimum_required(VERSION 3.20)

project(QDeltaPLC VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

find_package(Qt6 6.8 REQUIRED COMPONENTS
  Core
  Gui
  Quick
  Qml
  Network
  Charts
  Xml
)

qt_policy(SET QTP0004 NEW)
qt_standard_project_setup(REQUIRES 6.8)

option(
  QDELTA_FAST_QML_BUILD
  "Disable QML cache generation for faster development builds"
  ON
)

option(
  QDELTA_HOT_RELOAD
  "Build qdeltaplc as a Felgo Hot Reload client"
  OFF
)

set(QDELTA_QML_OPTIONS)

if(QDELTA_FAST_QML_BUILD)
  list(APPEND QDELTA_QML_OPTIONS NO_CACHEGEN)
endif()

set(QT_QML_OUTPUT_DIRECTORY
  "${CMAKE_BINARY_DIR}/qml"
)
set( QML_IMPORT_PATH
  "${QT_QML_OUTPUT_DIRECTORY};${CMAKE_CURRENT_SOURCE_DIR}/modules"
  CACHE STRING
  "QML import paths for Qt Creator"
  FORCE
)

qt_add_executable(qdeltaplc
  MANUAL_FINALIZATION
  src/main.cpp
)

add_subdirectory(src)
add_subdirectory(modules/Styles)
add_subdirectory(modules/Components)

set(QML_FILES
  qml/Main.qml
  qml/Logger.qml
  qml/NavigationPanel/NavigationPanel.qml
  qml/NavigationPanel/NavigationMenuDelegate.qml
  qml/Views/Dashboard/DeltaPanel/DeltaModuleAP.qml
  qml/Views/Dashboard/DeltaPanel/DeltaPanel.qml
  qml/Views/Dashboard/DeltaPanel/LedsPanel.qml
  qml/Views/Dashboard/DeltaPanel/DeltaModuleInput.qml
  qml/Views/Dashboard/DeltaPanel/DeltaModuleOutput.qml
  qml/Views/Dashboard/DeltaPanel/DeltaNetwork.qml
  qml/Views/Dashboard/DeltaPanel/KukaAutExt.qml
  qml/Views/Dashboard/FtsPanel/FtsPanel.qml
  qml/Views/Dashboard/FtsPanel/FtsBars.qml
  qml/Views/Dashboard/KukaPanel/KukaPanel.qml
  qml/Views/Dashboard/KukaPanel/RsiPosition.qml
  qml/Views/Dashboard/KukaPanel/RsiCoordinate.qml
  qml/Views/Dashboard/KukaPanel/Counter.qml
  qml/Views/Dashboard/Dashboard.qml
  qml/Views/Options/Options.qml
  qml/Views/Options/OptionsSection.qml
  qml/Views/Test/Test.qml
  qml/Views/Network/Network.qml
)

set(RESOURCE_FILES
  resources/pics.qrc
  resources/fonts.qrc
  resources/files.qrc
)

qt_add_qml_module(qdeltaplc
  URI qdeltaplc_qml_module
  VERSION 1.0
  ${QDELTA_QML_OPTIONS}
  IMPORTS
    Styles
    Components
  QML_FILES
    ${QML_FILES}
)

target_sources(qdeltaplc PRIVATE
  ${RESOURCE_FILES}
)

target_link_libraries(qdeltaplc PRIVATE
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

target_compile_definitions(qdeltaplc
  PRIVATE
    QDELTA_QML_MODULES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/modules"
)

if(QDELTA_HOT_RELOAD)
  set(QDELTA_HR_TARGET qdeltaplc)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FelgoHotReload.cmake)
endif()

qt_import_qml_plugins(qdeltaplc)
qt_finalize_executable(qdeltaplc)

include(GNUInstallDirs)

install(TARGETS qdeltaplc
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
