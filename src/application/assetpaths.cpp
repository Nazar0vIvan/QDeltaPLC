#include "assetpaths.h"

#include <QCoreApplication>
#include <QDir>

namespace RoboCrap3D {

ViewportAssets ViewportAssets::applicationAssets()
{
  const QDir executable(QCoreApplication::applicationDirPath());
  const QString cadDirectory = executable.filePath(QStringLiteral("resources/cad/kr10"));
  return {
    QStringLiteral(":/json/kr10.json"),
    cadDirectory,
    cadDirectory,
    executable.filePath(QStringLiteral("resources/occt"))
  };
}

} // namespace RoboCrap3D
