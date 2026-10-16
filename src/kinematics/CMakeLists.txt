qt_add_library(RoboCrapKinematics STATIC
  include/kinematics/kr10kinematicmodel.h
  include/kinematics/kr10kinematics.h
  kr10kinematics.cpp
)
target_include_directories(RoboCrapKinematics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(RoboCrapKinematics PUBLIC RoboCrapGeometry)
