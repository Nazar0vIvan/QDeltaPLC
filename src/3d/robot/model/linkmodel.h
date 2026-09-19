#pragma once

#include <QString>

#include "3d/occt/occpartprops.h"

namespace RoboCrap3D {

struct LinkModel
{
  QString fileName;
  OccPartProps props{};
};

} // namespace RoboCrap3D
