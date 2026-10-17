qt_add_library(RoboCrapGeometry STATIC
  include/geometry/boundedplane.h
  boundedplane.cpp
  include/geometry/boundedcylinder.h
  boundedcylinder.cpp
  include/geometry/surfaceintersection.h
  surfaceintersection.cpp
  include/geometry/plane.h
  plane.cpp
  include/geometry/mathtypes.h
  include/geometry/utils.h
  utils.cpp
  include/geometry/cylinder.h
  cylinder.cpp
  include/geometry/pose.h
  pose.cpp
)
target_include_directories(RoboCrapGeometry PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/libs/eigen-5.0.0
)
target_link_libraries(RoboCrapGeometry PUBLIC Qt6::Core)
