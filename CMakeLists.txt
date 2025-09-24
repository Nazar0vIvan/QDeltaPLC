cmake_minimum_required(VERSION 3.16)

project(QDeltaPLC VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.7 REQUIRED COMPONENTS Core Quick Qml Network Charts)

# With ver. 6.5 RESOURCE_PREFIX for qml modules is ":/qt/qml/"
qt_standard_project_setup(REQUIRES 6.5)

# set(QML_IMPORT_PATH ${CMAKE_BINARY_DIR}/qml/Modules CACHE STRING "" FORCE)

list(APPEND QML_IMPORT_PATH ${CMAKE_BINARY_DIR}/qml/Modules)
set(QML_IMPORT_PATH ${QML_IMPORT_PATH} CACHE STRING "" FORCE)

add_subdirectory(qml)
#add_subdirectory(src)
