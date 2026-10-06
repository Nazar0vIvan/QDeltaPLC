#include "cachedshapeloader.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <Standard_Failure.hxx>

#include <sstream>
#include <string>
#include <utility>

namespace RoboCrap3D {

CachedShapeLoader::CachedShapeLoader(QString cadDirectory, QString cacheDirectory)
  : m_cadDirectory(std::move(cadDirectory)), m_cacheDirectory(std::move(cacheDirectory))
{}

CadImportResult CachedShapeLoader::loadStpWithCache(const QString& fileName) const
{
  // Callers select a source directory; this argument is always its basename.
  if (fileName.isEmpty() || QFileInfo(fileName).fileName() != fileName
      || fileName.contains('/') || fileName.contains('\\')) {
    return CadImportResult::failure(QStringLiteral("Invalid CAD filename: %1").arg(fileName));
  }

  const QString suffix = QFileInfo(fileName).suffix().toLower();
  if (suffix != QStringLiteral("stp") && suffix != QStringLiteral("step"))
    return CadImportResult::failure(QStringLiteral("Choose a STEP or STP file: %1").arg(fileName));

  const QString baseName = QFileInfo(fileName).completeBaseName();
  if (baseName.isEmpty())
    return CadImportResult::failure(QStringLiteral("Invalid CAD filename: %1").arg(fileName));

  const QString sourcePath = QDir(m_cadDirectory).filePath(fileName);
  // Prefer the deployed BREP without opening or hashing the STEP source.
  const QString cachePath = QDir(m_cacheDirectory).filePath(baseName + QStringLiteral(".brep"));
  QFile cache(cachePath);
  if (!m_cacheDirectory.isEmpty() && cache.open(QIODevice::ReadOnly)) {
    try {
      const QByteArray bytes = cache.readAll();
      std::istringstream stream(std::string(bytes.constData(), static_cast<std::size_t>(bytes.size())));
      TopoDS_Shape shape;
      BRep_Builder builder;
      BRepTools::Read(shape, stream, builder);
      if (!shape.IsNull() && !stream.bad() && !stream.fail()) {
        qDebug() << "CAD loaded from BREP cache:" << cachePath;
        return CadImportResult::success(shape);
      }
    } catch (const Standard_Failure&) {
      // A derived cache is disposable; the STEP source remains authoritative.
    }
    qWarning() << "Invalid BREP cache; reimporting" << sourcePath;
  }
  cache.close();

  qDebug() << "Importing STEP source:" << sourcePath;
  const CadImportResult result = m_stepImporter.importFile(sourcePath);
  if (!result.ok) return result;

  if (!m_cacheDirectory.isEmpty() && QDir().mkpath(m_cacheDirectory)) {
    try {
      std::ostringstream stream;
      BRepTools::Write(result.shape, stream);
      const std::string bytes = stream.str();
      QSaveFile output(cachePath);
      if (!stream.good() || !output.open(QIODevice::WriteOnly)
          || output.write(bytes.data(), static_cast<qint64>(bytes.size())) != static_cast<qint64>(bytes.size())
          || !output.commit()) {
        qWarning() << "Cannot save BREP cache:" << cachePath;
      }
    } catch (const Standard_Failure& failure) {
      qWarning() << "Cannot serialize BREP cache:" << failure.what();
    }
  } else {
    qWarning() << "Cannot create BREP cache directory:" << m_cacheDirectory;
  }
  return result;
}

} // namespace RoboCrap3D
