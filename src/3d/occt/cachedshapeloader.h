#pragma once

#include "stepimporter.h"

namespace RoboCrap3D {

class CachedShapeLoader final
{
public:
  CachedShapeLoader(QString cadDirectory, QString cacheDirectory);
  CadImportResult loadStpWithCache(const QString& fileName) const;

private:
  QString m_cadDirectory;
  QString m_cacheDirectory;
  StepImporter m_stepImporter;
};

} // namespace RoboCrap3D
