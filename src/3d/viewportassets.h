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
  bool initializeOcct(QString& error) const;
};

} // namespace RoboCrap3D
