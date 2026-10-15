#pragma once

#include <QString>

namespace RoboCrap3D {

struct ViewportAssets
{
  QString modelFile;
  QString cadDirectory;
  QString cacheDirectory;
  QString occtResources;

  static ViewportAssets applicationAssets();
  QString initializeOcct() const;
};

} // namespace RoboCrap3D
