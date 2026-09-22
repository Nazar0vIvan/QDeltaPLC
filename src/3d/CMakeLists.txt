set(OCCT_ROOT "${PROJECT_SOURCE_DIR}/libs/OCCT_8_0_0/install-gcc1310_64"
    CACHE PATH "OCCT installation prefix")
if(NOT OpenCASCADE_DIR)
  set(OpenCASCADE_DIR "${OCCT_ROOT}/lib/cmake/opencascade")
endif()

# Keep the package's directory compile definitions inside this subsystem.
find_package(OpenCASCADE CONFIG REQUIRED)
if(NOT "${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION}" STREQUAL "8.0.0")
  message(FATAL_ERROR "The viewport integration requires the OCCT 8.0.0 SDK.")
endif()
if(NOT WIN32 OR NOT MINGW OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR NOT OpenCASCADE_COMPILER STREQUAL "gcc")
  message(FATAL_ERROR "Use a Windows 64-bit MinGW Qt kit with the bundled GCC OCCT SDK.")
endif()

foreach(occt_target IN LISTS OpenCASCADE_LIBRARIES)
  # The bundled SDK provides Release libraries only. The application may still
  # use its Debug Qt configuration with the compatible MinGW runtime.
  set_target_properties(${occt_target} PROPERTIES
    IMPORTED_GLOBAL TRUE
    MAP_IMPORTED_CONFIG_DEBUG Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
  )
endforeach()

qt_add_qml_module(RoboCrap3D
  STATIC
  URI RoboCrap.Viewport3D
  VERSION 1.0
  ${QML_OPTIONS}
  DEPENDENCIES QtQml QtQuick
  SOURCES
    viewportqmltypes.h
    occcontroller.h
    occcontroller.cpp
    viewportassets.h
    viewportassets.cpp
    math/mathtypes.h
    math/utils.h
    math/utils.cpp
    occt/cadloadworker.h
    occt/cadloadworker.cpp
    occt/cachedshapeloader.h
    occt/cachedshapeloader.cpp
    occt/stepimporter.h
    occt/stepimporter.cpp
    occt/occviewwindow.h
    occt/occviewwindow.cpp
    occt/occviewport.h
    occt/occviewport.cpp
    occt/occviewer.h
    occt/occviewer.cpp
    occt/occinputcontroller.h
    occt/occinputcontroller.cpp
    occt/occscene.h
    occt/occscene.cpp
    occt/occpart.h
    occt/occpart.cpp
    occt/occpartprops.h
    occt/occviewcube.h
    occt/occviewcube.cpp
    occt/occworldaxes.h
    occt/occworldaxes.cpp
    robot/robotpreviewstate.h
    robot/robotpreviewstate.cpp
    robot/model/jointmodel.h
    robot/model/linkmodel.h
    robot/model/kr10model.h
    robot/model/kr10model.cpp
    robot/kinematics/kr10kinematics.h
    robot/kinematics/kr10kinematics.cpp
    robot/visualization/robotoccsceneadapter.h
    robot/visualization/robotoccsceneadapter.cpp
)

target_include_directories(RoboCrap3D PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(RoboCrap3D
  PUBLIC Qt6::Core Qt6::Gui Qt6::Qml
  PRIVATE RoboCrapBackend TKernel TKMath TKG3d TKBRep TKTopAlgo TKService TKV3d TKOpenGl TKDESTEP
)

set(ROBOCRAP_OCCT_PREFIX "${OpenCASCADE_INSTALL_PREFIX}" PARENT_SCOPE)
