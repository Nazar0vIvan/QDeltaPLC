#include "cadloadworker.h"

#include "cachedshapeloader.h"

#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <Standard_Failure.hxx>

#include <exception>
#include <utility>

namespace RoboCrap3D {

CadLoadWorker::CadLoadWorker(QStringList sourcePaths, QString cacheDirectory)
  : m_sourcePaths(std::move(sourcePaths)), m_cacheDirectory(std::move(cacheDirectory))
{
  setObjectName(QStringLiteral("CadLoader"));
}

CadLoadResult CadLoadWorker::takeResult()
{
  Q_ASSERT(isFinished());
  return std::move(m_result);
}

void CadLoadWorker::run()
{
  // Independent requests retain the original serialized OCCT parsing/cache access.
  static QMutex importMutex;
  const QMutexLocker lock(&importMutex);
  try {
    for (const QString& path : std::as_const(m_sourcePaths)) {
      if (isInterruptionRequested()) return;
      const QFileInfo source(path);
      if (!source.isAbsolute()) {
        m_result.error = QStringLiteral("CAD source must be an absolute file path: %1").arg(path);
        return;
      }
      const CachedShapeLoader loader(source.absolutePath(), m_cacheDirectory);
      const CadImportResult result = loader.loadStpWithCache(source.fileName());
      if (isInterruptionRequested()) return;
      if (!result.ok) {
        m_result.error = result.error;
        return;
      }
      m_result.shapes.append(result.shape);
    }
  } catch (const Standard_Failure& failure) {
    m_result.error = QStringLiteral("CAD loading failed: %1").arg(QString::fromUtf8(failure.what()));
  } catch (const std::exception& failure) {
    m_result.error = QStringLiteral("CAD loading failed: %1").arg(QString::fromUtf8(failure.what()));
  }
}

} // namespace RoboCrap3D
