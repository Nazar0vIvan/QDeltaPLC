#pragma once

#include "cadloadresult.h"

#include <QStringList>
#include <QThread>

namespace RoboCrap3D {

// Only file/shape work runs here. The result is read after finished + wait().
class CadLoadWorker final : public QThread
{
public:
  CadLoadWorker(QStringList sourcePaths, QString cacheDirectory);
  CadLoadResult takeResult();

protected:
  void run() override;

private:
  QStringList m_sourcePaths;
  QString m_cacheDirectory;
  CadLoadResult m_result;
};

} // namespace RoboCrap3D
