set(frozen_rsi_sources
  ../pathgeneration/chamfer/chamferjob.cpp
  ../pathgeneration/chamfer/chamfercartesiansampler.cpp
  ../pathgeneration/chamfer/rsiposeencoder.cpp
  ../pathgeneration/chamfer/rsicorrectionlimits.cpp
  ../pathgeneration/rsi/offsetjsonwriter.cpp
  ../pathgeneration/rsi/rsipath.cpp
)

qt_add_library(RoboCrapTrajectories STATIC
  include/trajectories/chamferpath.h
  chamfer/chamferpath.cpp
  include/trajectories/chamferarclength.h
  chamfer/chamferarclength.cpp
  include/trajectories/chamfercentralmotion.h
  chamfer/chamfercentralmotion.cpp
  chamfer/chamfercentralvalidation.h
  chamfer/chamfercentralvalidation.cpp
  chamfer/chamferpreparationdiagnostics.h
  include/trajectories/chamfermotion.h
  chamfer/chamfermotion.cpp
  # Frozen RSI implementation remains at its legacy paths.
  ${frozen_rsi_sources}
  ../pathgeneration/chamfer/chamferjob.h
  ../pathgeneration/chamfer/chamfercartesiansampler.h
  ../pathgeneration/chamfer/rsiposeencoder.h
  ../pathgeneration/chamfer/rsicorrectionlimits.h
  ../pathgeneration/rsi/motionprofile.h
  ../pathgeneration/rsi/offsetjsonwriter.h
  ../pathgeneration/rsi/rsipath.h
)
target_include_directories(RoboCrapTrajectories PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(RoboCrapTrajectories PUBLIC RoboCrapKinematics)
# Only frozen RSI translation units need includes rooted at the legacy src directory.
set_source_files_properties(${frozen_rsi_sources}
  PROPERTIES INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/src"
)
