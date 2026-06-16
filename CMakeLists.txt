cmake_minimum_required(VERSION 3.16)

project(QDeltaPLC VERSION 0.1 LANGUAGES CXX)

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

qt_standard_project_setup(REQUIRES 6.8)

option(QDELTAPLC_FAST_BUILD "Speed up local builds by disabling expensive QML build steps" ON)

if(QDELTAPLC_FAST_BUILD)
  set(QT_QML_NO_CACHEGEN TRUE)
endif()

qt_add_executable(qdeltaplc
  src/main.cpp
)

add_subdirectory(src)
add_subdirectory(qml/Modules/Styles)
add_subdirectory(qml/Modules/Components)

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
)

set(RESOURCE_FILES
  qml/resources/pics.qrc
  qml/resources/fonts.qrc
)

qt_add_qml_module(qdeltaplc
  URI qdeltaplc_qml_module
  VERSION 1.0
  DEPENDENCIES
    TARGET Styles
    TARGET Components
  QML_FILES
    ${QML_FILES}
)

target_sources(qdeltaplc
  PRIVATE
    ${RESOURCE_FILES}
)

target_link_libraries(qdeltaplc
  PRIVATE
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

include(GNUInstallDirs)

install(TARGETS qdeltaplc
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)