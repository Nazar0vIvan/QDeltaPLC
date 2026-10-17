#pragma once

#include <QString>

namespace RoboCrap3D {

struct ViewportAssets
{
  QString cadDirectory;
  QString occtResources;

  static ViewportAssets applicationAssets();
};

} // namespace RoboCrap3D
