#include "stepimporter.h"

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>

#include <IFSelect_ReturnStatus.hxx>

#include <STEPControl_Reader.hxx>

namespace RoboCrap3D {

CadImportResult StepImporter::importFile(const QString& filePath) const
{
  if (filePath.trimmed().isEmpty()) {
    return CadImportResult::failure(
      QStringLiteral("STEP import failed: file path is empty")
    );
  }

  const QFileInfo fileInfo(filePath);

  if (!fileInfo.exists()) {
    return CadImportResult::failure(
      QStringLiteral("STEP import failed: file does not exist: %1").arg(filePath)
    );
  }

  if (!fileInfo.isFile()) {
    return CadImportResult::failure(
      QStringLiteral("STEP import failed: path is not a file: %1").arg(filePath)
    );
  }

  const QByteArray nativePathBytes = toNativePathBytes(fileInfo.absoluteFilePath());

  STEPControl_Reader reader;

  QElapsedTimer timer;
  timer.start();

  const IFSelect_ReturnStatus readStatus = reader.ReadFile(nativePathBytes.constData());
  qDebug() << "STEP ReadFile ms:" << timer.elapsed();

  if (readStatus != IFSelect_RetDone) {
    return CadImportResult::failure(
      QStringLiteral("STEP import failed: cannot read file: %1. Status: %2")
        .arg(fileInfo.absoluteFilePath(), statusToString(readStatus))
    );
  }

  timer.restart();
  const int transferredRoots = reader.TransferRoots();
  qDebug() << "STEP TransferRoots ms:" << timer.elapsed();

  if (transferredRoots <= 0) {
    return CadImportResult::failure(QStringLiteral("STEP import failed: no transferable roots found: %1")
      .arg(fileInfo.absoluteFilePath())
    );
  }

  timer.restart();
  TopoDS_Shape importedShape = reader.OneShape();
  qDebug() << "STEP OneShape ms:" << timer.elapsed();


  if (importedShape.IsNull()) {
    return CadImportResult::failure(
      QStringLiteral("STEP import failed: resulting shape is null: %1")
        .arg(fileInfo.absoluteFilePath())
      );
  }

  return CadImportResult::success(importedShape);
}

QByteArray StepImporter::toNativePathBytes(const QString& filePath)
{
  const QString nativePath = QDir::toNativeSeparators(filePath);

  // OCCT's narrow file APIs use UTF-8, including on Windows.
  return nativePath.toUtf8();
}

QString StepImporter::statusToString(const IFSelect_ReturnStatus status)
{
  switch (status) {
    case IFSelect_RetVoid:  return QStringLiteral("IFSelect_RetVoid");
    case IFSelect_RetDone:  return QStringLiteral("IFSelect_RetDone");
    case IFSelect_RetError: return QStringLiteral("IFSelect_RetError");
    case IFSelect_RetFail:  return QStringLiteral("IFSelect_RetFail");
    case IFSelect_RetStop:  return QStringLiteral("IFSelect_RetStop");
    }

  return QStringLiteral("Unknown IFSelect_ReturnStatus(%1)").arg(static_cast<int>(status));
}

} // namespace RoboCrap3D
