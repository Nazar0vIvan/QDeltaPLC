set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EIGEN5_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs/eigen-5.0.0)

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

  network/common/socketstateutils.h
  network/common/socketstateutils.cpp

  network/fts/rdtmessage.h
  network/fts/socketfts.h
  network/fts/socketfts.cpp

  network/plc/plcmessagemanager.h
  network/plc/plcmessagemanager.cpp
  network/plc/socketdeltaplc.h
  network/plc/socketdeltaplc.cpp

  network/rsi/socketrsi.h
  network/rsi/socketrsi.cpp

  network/runner/abstractsocketrunner.h
  network/runner/abstractsocketrunner.cpp
  network/runner/ftsrunner.h
  network/runner/ftsrunner.cpp
  network/runner/plcrunner.h
  network/runner/plcrunner.cpp
  network/runner/rsirunner.h
  network/runner/rsirunner.cpp
  network/runner/tcpsocketrunner.h
  network/runner/tcpsocketrunner.cpp
  network/runner/udpsocketrunner.h
  network/runner/udpsocketrunner.cpp

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

  logger.h
  logger.cpp
  qmlchartbridge.h
  qmlchartbridge.cpp
)

target_include_directories(qdeltaplc PRIVATE
  ${SRC_DIR}

  ${SRC_DIR}/network
  ${SRC_DIR}/network/common
  ${SRC_DIR}/network/fts
  ${SRC_DIR}/network/plc
  ${SRC_DIR}/network/rsi
  ${SRC_DIR}/network/runner

  ${SRC_DIR}/geometry

  ${SRC_DIR}/pathgeneration
  ${SRC_DIR}/pathgeneration/blade
  ${SRC_DIR}/pathgeneration/rsi

  ${EIGEN5_INCLUDE_DIR}
)
