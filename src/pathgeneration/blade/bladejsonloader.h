#pragma once

#include <QString>
#include <QVariantMap>

#include "airfoil.h"

class BladeJsonLoader
{
public:
  struct LoadResult
  {
    QString path;
    Airfoil airfoil;
    bool ok = false;
    QString error;
  };
  // Expects {"path": QUrl}
  static LoadResult load(const QVariantMap& data);
  // Direct file path entry point
  static LoadResult loadFromFile(const QString& path);
};