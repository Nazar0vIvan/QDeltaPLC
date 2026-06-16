set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EIGEN5_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libs/eigen-5.0.0)

target_sources(qdeltaplc
  PRIVATE
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
)

target_include_directories(qdeltaplc
  PRIVATE
    ${SRC_DIR}
    ${SRC_DIR}/network
    ${EIGEN5_INCLUDE_DIR}
)
