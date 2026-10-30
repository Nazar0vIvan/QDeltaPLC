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
  Widgets
)

option(
  FAST_QML_BUILD
  "Disable QML cache generation for faster development builds"
  ON
)

option(
  USE_HOT_RELOAD
  "Enable Felgo Hot Reload"
  OFF
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

add_subdirectory(src/geometry)
add_subdirectory(src/kinematics)
add_subdirectory(src/trajectories)
add_subdirectory(src/devices)
add_subdirectory(src/viewport)
# Uses the OCCT imported targets made global by src/viewport.
add_subdirectory(tools/cadstep2brep)
add_subdirectory(qml/Modules/Styles)
add_subdirectory(qml/Modules/Components)

include(GNUInstallDirs)
include(cmake/DeployOcct.cmake)
add_subdirectory(src/application)

install(TARGETS robocrap
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
