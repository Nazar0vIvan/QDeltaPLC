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

qt_policy(SET QTP0004 NEW)

option(QDELTAPLC_FAST_BUILD "Speed up local builds by disabling expensive QML build steps" ON)

if(QDELTAPLC_FAST_BUILD)
  set(QT_QML_NO_CACHEGEN TRUE)
endif()

# Mostly useful for Qt Creator / qmlls.
list(APPEND QML_IMPORT_PATH ${CMAKE_BINARY_DIR}/qml/Modules)
set(QML_IMPORT_PATH ${QML_IMPORT_PATH} CACHE STRING "" FORCE)

qt_add_executable(qdeltaplc
  src/main.cpp
)

add_subdirectory(src)
add_subdirectory(qml)

target_link_libraries(qdeltaplc
  PRIVATE
    Qt6::Gui
    Qt6::Quick
    Qt6::Network
    Qt6::Charts
    Qt6::Xml
    Qt6::Widgets
)

include(GNUInstallDirs)

install(TARGETS qdeltaplc
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
