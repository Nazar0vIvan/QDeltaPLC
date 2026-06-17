set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EIGEN5_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs/eigen-5.0.0)

target_sources(qdeltaplc PRIVATE
  logger.h
  logger.cpp
  qmlchartbridge.h
  qmlchartbridge.cpp
  network/socketdeltaplc.h
  network/socketdeltaplc.cpp
  network/socketfts.h
  network/socketfts.cpp
  network/socketrsi.h
  network/socketrsi.cpp
  network/socketrunner.h
  network/socketrunner.cpp
  network/plcmessagemanager.h
  network/plcmessagemanager.cpp
  geometry/mathtypes.h
  geometry/utils.h
  geometry/utils.cpp
  geometry/pose.h
  geometry/pose.cpp
  geometry/plane.h
  geometry/plane.cpp
  geometry/cylinder.h
  geometry/cylinder.cpp
  pathgeneration/blade.h
  pathgeneration/blade.cpp
  pathgeneration/planemesh.h
  pathgeneration/planemesh.cpp
  pathgeneration/rsi.h
  pathgeneration/rsi.cpp
)

target_include_directories(qdeltaplc PRIVATE
  ${SRC_DIR}
  ${SRC_DIR}/network
  ${SRC_DIR}/geometry
  ${SRC_DIR}/pathgeneration
  ${EIGEN5_INCLUDE_DIR}
)
