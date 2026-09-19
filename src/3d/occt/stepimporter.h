#pragma once

#include <QByteArray>
#include <QString>
#include <TopoDS_Shape.hxx>

#include <IFSelect_ReturnStatus.hxx>

namespace RoboCrap3D {

struct CadImportResult
{
  bool ok = false;
  TopoDS_Shape shape;
  QString error;

  [[nodiscard]] static CadImportResult success(const TopoDS_Shape& importedShape)
  {
    CadImportResult result;
    result.ok = true;
    result.shape = importedShape;
    return result;
  }

  [[nodiscard]] static CadImportResult failure(const QString& message)
  {
    CadImportResult result;
    result.ok = false;
    result.error = message;
    return result;
  }
};

class StepImporter final
{
public:
  StepImporter() = default;
  ~StepImporter() = default;

  StepImporter(const StepImporter&) = default;
  StepImporter& operator=(const StepImporter&) = default;

  StepImporter(StepImporter&&) noexcept = default;
  StepImporter& operator=(StepImporter&&) noexcept = default;

  CadImportResult importFile(const QString& filePath) const;

private:
  static QByteArray toNativePathBytes(const QString& filePath);
  static QString statusToString(IFSelect_ReturnStatus status);
};

} // namespace RoboCrap3D
