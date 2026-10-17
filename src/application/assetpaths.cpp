#include "assetpaths.h"

#include <QCoreApplication>
#include <QDir>

namespace RoboCrap3D {

ViewportAssets ViewportAssets::applicationAssets()
{
  const QDir executable(QCoreApplication::applicationDirPath());
  return {
    executable.filePath(QStringLiteral("resources/cad/kr10")),
    executable.filePath(QStringLiteral("resources/occt"))
  };
}

} // namespace RoboCrap3D
