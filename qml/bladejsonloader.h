#ifndef BLADEJSONLOADER_H
#define BLADEJSONLOADER_H

#include <QVariantMap>
#include <QString>

#include "pathplanner.h" // Airfoil, BladeProfile, V3d

class BladeJsonLoader
{
public:
  struct LoadResult {
    QString path;
    Airfoil airfoil;
    bool ok = false;
    QString error;
  };

  // Accepts the same input as SocketRSI::loadBladeJson (expects {"path": QUrl})
  static LoadResult load(const QVariantMap& data);

  // Direct file-path entry point (useful for non-QML callers/tests)
  static LoadResult loadFromFile(const QString& path);
};

#endif // BLADEJSONLOADER_H
