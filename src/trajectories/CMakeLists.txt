qt_add_library(RoboCrapTrajectories STATIC
  motion/jointsegments.h
  motion/jointsegments.cpp
  include/trajectories/timedmotion.h
  motion/timedmotion.cpp
  motion/tcpsolver.h
  motion/motionbuilder.h
  motion/motionbuilder.cpp
  include/trajectories/chamferpath.h
  chamfer/chamferpath.cpp
  include/trajectories/chamferarclength.h
  chamfer/chamferarclength.cpp
  include/trajectories/chamfercentralmotion.h
  chamfer/chamfercentralmotion.cpp
  chamfer/chamfercentralvalidation.h
  chamfer/chamfercentralvalidation.cpp
  include/trajectories/chamfermotion.h
  chamfer/chamfermotion.cpp
  include/trajectories/chamferrsistream.h
  chamfer/chamferrsistream.cpp
  include/trajectories/probetouchplan.h
  probe/probetouchplan.cpp
  include/trajectories/probetouchmotion.h
  probe/probetouchmotion.cpp
)
target_include_directories(RoboCrapTrajectories PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(RoboCrapTrajectories PUBLIC RoboCrapKinematics)
