set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EIGEN5_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs/eigen-5.0.0)

qt_add_library(RoboCrapGeometry STATIC
  3d/robot/model/kr10kinematicmodel.h
  3d/robot/kinematics/kr10kinematics.h
  3d/robot/kinematics/kr10kinematics.cpp
  geometry/boundedplane.h
  geometry/boundedplane.cpp
  geometry/boundedcylinder.h
  geometry/boundedcylinder.cpp
  geometry/circle.h
  geometry/circle.cpp
  geometry/surfaceintersection.h
  geometry/surfaceintersection.cpp
  geometry/plane.h
  geometry/plane.cpp
  geometry/mathtypes.h
  geometry/utils.h
  geometry/utils.cpp
  geometry/cylinder.h
  geometry/cylinder.cpp
  geometry/pose.h
  geometry/pose.cpp
  pathgeneration/blade/airfoil.h
  pathgeneration/blade/bladefrenet.h
  pathgeneration/blade/bladefrenet.cpp
  pathgeneration/blade/bladejsonloader.h
  pathgeneration/blade/bladejsonloader.cpp
  pathgeneration/chamfer/chamferpath.h
  pathgeneration/chamfer/chamferpath.cpp
  pathgeneration/chamfer/chamfermotion.h
  pathgeneration/chamfer/chamfermotion.cpp
  pathgeneration/chamfer/chamferjob.h
  pathgeneration/chamfer/chamferjob.cpp
  pathgeneration/chamfer/chamfercartesiansampler.h
  pathgeneration/chamfer/chamfercartesiansampler.cpp
  pathgeneration/chamfer/rsiposeencoder.h
  pathgeneration/chamfer/rsiposeencoder.cpp
  pathgeneration/chamfer/rsicorrectionlimits.h
  pathgeneration/chamfer/rsicorrectionlimits.cpp
  pathgeneration/rsi/motionprofile.h
  pathgeneration/rsi/offsetjsonwriter.h
  pathgeneration/rsi/offsetjsonwriter.cpp
  pathgeneration/rsi/rsipath.h
  pathgeneration/rsi/rsipath.cpp
)
target_include_directories(RoboCrapGeometry PUBLIC ${SRC_DIR} ${EIGEN5_INCLUDE_DIR})
target_link_libraries(RoboCrapGeometry PUBLIC Qt6::Core)

qt_add_library(RoboCrapScene STATIC
  scene/sceneendeffectors.h
  scene/sceneendeffectors.cpp
  scene/scenemachining.h
  scene/scenemachining.cpp
  scene/scenemachiningpreparation.h
  scene/scenemachiningpreparation.cpp
  scene/scenemodel.h
  scene/scenemodel.cpp
  scene/sceneobject.h
  scene/sceneobject.cpp
  scene/scenegeometry.h
  scene/scenesurfaceimporter.h
  scene/scenesurfaceimporter.cpp
)
target_include_directories(RoboCrapScene PUBLIC ${SRC_DIR})
target_link_libraries(RoboCrapScene PUBLIC RoboCrapGeometry Qt6::Qml)
qt_extract_metatypes(RoboCrapScene)

qt_add_library(RoboCrapDevices STATIC
  network/abstractdevice.h
  network/abstractdevice.cpp
  network/devicerunner.h
  network/devicerunner.cpp
  network/devicehub.h
  network/devicehub.cpp
  network/common/socketstateutils.h
  network/common/socketstateutils.cpp
  network/common/socketconfigutils.h
  network/common/deviceprofilemodel.h
  network/common/deviceprofilemodel.cpp
  network/plc/plcmessagemanager.h
  network/plc/plcmessagemanager.cpp
  network/fts/rdtmessage.h
  network/fts/ftsdevice.h
  network/fts/ftsdevice.cpp
  network/plc/plcdevice.h
  network/plc/plcdevice.cpp
  network/rsi/rsidevice.h
  network/rsi/rsidevice.cpp
  network/rsi/rsiprotocol.h
  network/rsi/rsiprotocol.cpp
  network/rsi/rsiholeprotocol.h
  network/rsi/rsiholeprotocol.cpp
  network/common/loggermessage.h
)
target_include_directories(RoboCrapDevices PUBLIC ${SRC_DIR})
target_link_libraries(RoboCrapDevices PUBLIC RoboCrapGeometry Qt6::Network Qt6::Qml)
qt_extract_metatypes(RoboCrapDevices)

qt_add_qml_module(RoboCrapBackend
  STATIC
  URI RoboCrap.Backend
  VERSION 1.0
  DEPENDENCIES QtQml
  SOURCES backendqmltypes.h
)

# Foreign-type registration needs the original headers by their moc include names.
target_include_directories(RoboCrapBackend PRIVATE
  ${SRC_DIR}/scene
  ${SRC_DIR}/network
  ${SRC_DIR}/network/common
  ${SRC_DIR}/network/plc
)
target_link_libraries(RoboCrapBackend PUBLIC RoboCrapScene RoboCrapDevices)

add_subdirectory(3d)
set(ROBOCRAP_OCCT_PREFIX "${ROBOCRAP_OCCT_PREFIX}" PARENT_SCOPE)

target_sources(robocrap PRIVATE
  logger.h
  logger.cpp
  qmlchartbridge.h
  qmlchartbridge.cpp
)
target_include_directories(robocrap PRIVATE ${SRC_DIR})
