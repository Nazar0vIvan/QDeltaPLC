#include "cachedshapeloader.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>

#include <sstream>
#include <string>
#include <utility>

namespace RoboCrap3D {

CachedShapeLoader::CachedShapeLoader(QString cadDirectory, QString cacheDirectory)
  : m_cadDirectory(std::move(cadDirectory)), m_cacheDirectory(std::move(cacheDirectory))
{}

CadImportResult CachedShapeLoader::loadStpWithCache(const QString& fileName) const
{
  // The robot manifest lists filenames, not paths outside its asset directory.
  if (fileName.isEmpty() || QFileInfo(fileName).fileName() != fileName
      || fileName.contains('/') || fileName.contains('\\')) {
    return CadImportResult::failure(QStringLiteral("Invalid robot CAD filename: %1").arg(fileName));
  }

  const QString sourcePath = QDir(m_cadDirectory).filePath(fileName);
  QFile source(sourcePath);
  if (!source.open(QIODevice::ReadOnly)) {
    return CadImportResult::failure(QStringLiteral("Cannot read %1: %2").arg(sourcePath, source.errorString()));
  }
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(QByteArrayLiteral("robocrap-step-cache-v1:" OCC_VERSION_COMPLETE));
  if (!hash.addData(&source)) {
    return CadImportResult::failure(QStringLiteral("Cannot hash CAD file: %1").arg(sourcePath));
  }
  source.close();
  const QString cachePath = QDir(m_cacheDirectory).filePath(QString::fromLatin1(hash.result().toHex())
                                                         + QStringLiteral(".brep"));
  QFile cache(cachePath);
  if (!m_cacheDirectory.isEmpty() && cache.open(QIODevice::ReadOnly)) {
    try {
      const QByteArray bytes = cache.readAll();
      std::istringstream stream(std::string(bytes.constData(), static_cast<std::size_t>(bytes.size())));
      TopoDS_Shape shape;
      BRep_Builder builder;
      BRepTools::Read(shape, stream, builder);
      if (!shape.IsNull() && !stream.bad() && !stream.fail()) {
        return CadImportResult::success(shape);
      }
    } catch (const Standard_Failure&) {
      // A derived cache is disposable; the STEP source remains authoritative.
    }
    qWarning() << "Invalid BREP cache; reimporting" << sourcePath;
  }
  cache.close();

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
