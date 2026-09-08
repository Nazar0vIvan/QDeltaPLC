set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EIGEN5_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs/eigen-5.0.0)

qt_add_qml_module(QDeltaBackend
  STATIC
  URI QDelta.Backend
  VERSION 1.0

  DEPENDENCIES
    QtQml

  SOURCES
    network/backendqmltypes.h

    network/abstractdevice.h
    network/abstractdevice.cpp
    network/devicerunner.h
    network/devicerunner.cpp
    network/devicehub.h
    network/devicehub.cpp

    network/common/socketstateutils.h
    network/common/socketstateutils.cpp
    network/common/deviceprofilemodel.h
    network/common/deviceprofilemodel.cpp

    network/plc/plcmessagemanager.h
    network/plc/plcmessagemanager.cpp

    logger.h
    logger.cpp
)

target_include_directories(QDeltaBackend PUBLIC
  ${SRC_DIR}
  ${SRC_DIR}/network
  ${SRC_DIR}/network/common
  ${SRC_DIR}/network/plc
)

target_link_libraries(QDeltaBackend PUBLIC
  Qt6::Core
  Qt6::Qml
  Qt6::Network
)

target_sources(qdeltaplc PRIVATE
  geometry/cylinder.h
  geometry/cylinder.cpp
  geometry/mathtypes.h
  geometry/plane.h
  geometry/plane.cpp
  geometry/pose.h
  geometry/pose.cpp
  geometry/utils.h
  geometry/utils.cpp

  network/fts/rdtmessage.h
  network/fts/ftsdevice.h
  network/fts/ftsdevice.cpp

  network/plc/plcdevice.h
  network/plc/plcdevice.cpp

  network/rsi/rsidevice.h
  network/rsi/rsidevice.cpp

  pathgeneration/blade/airfoil.h
  pathgeneration/blade/bladefrenet.h
  pathgeneration/blade/bladefrenet.cpp
  pathgeneration/blade/bladejsonloader.h
  pathgeneration/blade/bladejsonloader.cpp
  pathgeneration/rsi/motionprofile.h
  pathgeneration/rsi/offsetjsonwriter.h
  pathgeneration/rsi/offsetjsonwriter.cpp
  pathgeneration/rsi/rsipath.h
  pathgeneration/rsi/rsipath.cpp

  qmlchartbridge.h
  qmlchartbridge.cpp
)

target_include_directories(qdeltaplc PRIVATE
  ${SRC_DIR}

  ${SRC_DIR}/geometry

  ${SRC_DIR}/network
  ${SRC_DIR}/network/common
  ${SRC_DIR}/network/fts
  ${SRC_DIR}/network/plc
  ${SRC_DIR}/network/rsi

  ${SRC_DIR}/pathgeneration
  ${SRC_DIR}/pathgeneration/blade
  ${SRC_DIR}/pathgeneration/rsi

  ${EIGEN5_INCLUDE_DIR}
)
