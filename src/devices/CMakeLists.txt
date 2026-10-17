qt_add_library(RoboCrapDevices STATIC
  abstractdevice.cpp
  common/socketconfigutils.h
  fts/ftsdevice.cpp
  include/devices/abstractdevice.h
  include/devices/fts/ftsdevice.h
  include/devices/fts/rdtmessage.h
  include/devices/plc/plcdevice.h
  include/devices/plc/plcmessagemanager.h
  include/devices/rsi/rsidevice.h
  plc/plcdevice.cpp
  plc/plcmessagemanager.cpp
  rsi/rsidevice.cpp
)
target_include_directories(RoboCrapDevices
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
# Geometry supplies V6d for the public RSI correction stream.
target_link_libraries(RoboCrapDevices PUBLIC RoboCrapGeometry Qt6::Network)
qt_extract_metatypes(RoboCrapDevices)
