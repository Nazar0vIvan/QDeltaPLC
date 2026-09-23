#include "viewportassets.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include <Font_FontMgr.hxx>
#include <Standard_Failure.hxx>

namespace RoboCrap3D {

namespace {

void setResource(const QDir& resources, const char* variable, const QString& directory)
{
  qputenv(variable, resources.filePath(directory).toUtf8());
}

} // namespace

ViewportAssets ViewportAssets::applicationAssets()
{
  const QDir executable(QCoreApplication::applicationDirPath());
  const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  return {
    QStringLiteral(":/json/kr10.json"),
    executable.filePath(QStringLiteral("resources/cad/kr10")),
    cacheRoot.isEmpty() ? QString{} : QDir(cacheRoot).filePath(QStringLiteral("occt/kr10")),
    executable.filePath(QStringLiteral("resources/occt"))
  };
}

bool ViewportAssets::initializeOcct(QString& error) const
{
  const QDir resources(occtResources);
  if (!resources.exists(QStringLiteral("Shaders")) || !resources.exists(QStringLiteral("XSTEPResource"))) {
    error = QStringLiteral("OCCT runtime resources are missing: %1").arg(occtResources);
    return false;
  }

  setResource(resources, "CSF_ShadersDirectory", QStringLiteral("Shaders"));
  setResource(resources, "CSF_MDTVTexturesDirectory", QStringLiteral("Textures"));
  setResource(resources, "CSF_SHMessage", QStringLiteral("SHMessage"));
  setResource(resources, "CSF_XSMessage", QStringLiteral("XSMessage"));
  setResource(resources, "CSF_STEPDefaults", QStringLiteral("XSTEPResource"));
  setResource(resources, "CSF_PluginDefaults", QStringLiteral("StdResource"));
  setResource(resources, "CSF_StandardDefaults", QStringLiteral("StdResource"));
  setResource(resources, "CSF_XCAFDefaults", QStringLiteral("StdResource"));
  try {
    Font_FontMgr::GetInstance()->InitFontDataBase();
    return true;
  } catch (const Standard_Failure& failure) {
    error = QStringLiteral("Cannot initialize OCCT: %1").arg(QString::fromUtf8(failure.what()));
    return false;
  }
}

} // namespace RoboCrap3D
