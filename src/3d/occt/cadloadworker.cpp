#include "cadloadworker.h"

#include "cachedshapeloader.h"

#include <Standard_Failure.hxx>

#include <exception>
#include <utility>

namespace RoboCrap3D {

CadLoadWorker::CadLoadWorker(Kr10Model model, ViewportAssets assets)
  : m_model(std::move(model)), m_assets(std::move(assets))
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
  try {
    const CachedShapeLoader loader(m_assets.cadDirectory, m_assets.cacheDirectory);
    for (std::size_t i = 0; i < LinkCount; ++i) {
      if (isInterruptionRequested()) return;
      const CadImportResult result = loader.loadStpWithCache(m_model.links[i].fileName);
      if (!result.ok) {
        m_result.error = result.error;
        return;
      }
      m_result.links[i] = result.shape;
    }
    if (isInterruptionRequested()) return;
    try {
      const CadImportResult effector = loader.loadStpWithCache(m_model.endEffector.fileName);
      if (effector.ok) {
        m_result.endEffector = effector.shape;
      } else {
        m_result.warning = QStringLiteral("End effector is unavailable: %1").arg(effector.error);
      }
    } catch (const Standard_Failure& failure) {
      m_result.warning = QStringLiteral("End effector is unavailable: %1")
                             .arg(QString::fromUtf8(failure.what()));
    } catch (const std::exception& failure) {
      m_result.warning = QStringLiteral("End effector is unavailable: %1")
                             .arg(QString::fromUtf8(failure.what()));
    }
  } catch (const Standard_Failure& failure) {
    m_result.error = QStringLiteral("CAD loading failed: %1").arg(QString::fromUtf8(failure.what()));
  } catch (const std::exception& failure) {
    m_result.error = QStringLiteral("CAD loading failed: %1").arg(QString::fromUtf8(failure.what()));
  }
}

} // namespace RoboCrap3D
