#pragma once

#include "kinematics/kr10kinematicmodel.h"
#include "viewport/robotgraphics.h"

namespace RoboCrap3D {

struct RobotDefinition
{
  Kr10KinematicModel kinematics;
  RobotVisualModel visuals;

  // Fixed KUKA KR10 R1420 nominal geometry and the deployed link CAD files.
  static RobotDefinition kr10();
};
} // namespace RoboCrap3D
