#pragma once

#include <limits>
#include <QString>

#include "3d/math/mathtypes.h"

namespace RoboCrap3D {

struct JointModel
{
  V3d axis{0.0, 0.0, 1.0};

  // transform from current i-th JCS to the previous (i-1)-th JSC
  M4d localTransform{};

  double qMin = -std::numeric_limits<double>::infinity();
  double qMax =  std::numeric_limits<double>::infinity();
};



} // namespace RoboCrap3D
