qt_add_library(RoboCrapDevices STATIC
  abstractdevice.cpp
  common/socketconfigutils.h
  fts/ftsdevice.cpp
  include/devices/abstractdevice.h
  include/devices/fts/ftsdevice.h
  include/devices/fts/rdtmessage.h
  include/devices/plc/plcdevice.h
  include/devices/plc/plcmessagemanager.h
  plc/plcdevice.cpp
  plc/plcmessagemanager.cpp
  ../network/rsi/rsidevice.h
  ../network/rsi/rsidevice.cpp
  ../network/rsi/rsiprotocol.h
  ../network/rsi/rsiprotocol.cpp
  ../network/rsi/rsiholeprotocol.h
  ../network/rsi/rsiholeprotocol.cpp
)
target_include_directories(RoboCrapDevices
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    # Frozen RSI headers are also read by generated moc compilation.
    ${PROJECT_SOURCE_DIR}/src
)
# Public numerical types remain solely for the frozen RSI interface.
target_link_libraries(RoboCrapDevices PUBLIC RoboCrapTrajectories Qt6::Network)
qt_extract_metatypes(RoboCrapDevices)
