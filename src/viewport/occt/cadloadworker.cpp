#include "cadloadworker.h"

#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <Standard_Failure.hxx>

#include <exception>
#include <sstream>
#include <string>
#include <utility>

namespace RoboCrap3D {

CadLoadWorker::CadLoadWorker(QStringList sourcePaths)
  : m_sourcePaths(std::move(sourcePaths))
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
  // Independent requests retain the original serialized OCCT parsing.
  static QMutex importMutex;
  const QMutexLocker lock(&importMutex);
  try {
    for (const QString& path : std::as_const(m_sourcePaths)) {
      if (isInterruptionRequested()) return;
      if (!QFileInfo(path).isAbsolute()) {
        m_result.error = QStringLiteral("CAD source must be an absolute file path: %1").arg(path);
        return;
      }
      // Read through QFile so non-ASCII Windows paths reach OCCT as bytes.
      QFile file(path);
      TopoDS_Shape shape;
      if (file.open(QIODevice::ReadOnly)) {
        const QByteArray bytes = file.readAll();
        std::istringstream stream(std::string(bytes.constData(), static_cast<std::size_t>(bytes.size())));
        BRep_Builder builder;
        BRepTools::Read(shape, stream, builder);
        if (stream.bad() || stream.fail()) shape.Nullify();
      }
      if (isInterruptionRequested()) return;
      if (shape.IsNull()) {
        m_result.error = QStringLiteral("Cannot read CAD file: %1").arg(path);
        return;
      }
      m_result.shapes.append(shape);
    }
  } catch (const Standard_Failure& failure) {
    m_result.error = QStringLiteral("CAD loading failed: %1").arg(QString::fromUtf8(failure.what()));
  } catch (const std::exception& failure) {
    m_result.error = QStringLiteral("CAD loading failed: %1").arg(QString::fromUtf8(failure.what()));
  }
}

} // namespace RoboCrap3D
