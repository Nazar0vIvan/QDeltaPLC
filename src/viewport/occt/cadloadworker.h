#pragma once

#include "cadloadresult.h"

#include <QStringList>
#include <QThread>

namespace RoboCrap3D {

// Only file/shape work runs here. The result is read after finished + wait().
class CadLoadWorker final : public QThread
{
public:
  // Absolute BREP file paths, loaded in order.
  explicit CadLoadWorker(QStringList sourcePaths);
  CadLoadResult takeResult();

protected:
  void run() override;

private:
  QStringList m_sourcePaths;
  CadLoadResult m_result;
};

} // namespace RoboCrap3D
