#include "viewportassets.h"

#include <QCoreApplication>
#include <QDir>
#include <utility>

#include <Font_FontMgr.hxx>
#include <Standard_Failure.hxx>

namespace RoboCrap3D {

ViewportAssets ViewportAssets::applicationAssets()
{
  const QDir executable(QCoreApplication::applicationDirPath());
  const QString cadDirectory = executable.filePath(QStringLiteral("resources/cad/kr10"));
  return {
    QStringLiteral(":/json/kr10.json"),
    cadDirectory,
    cadDirectory,
    executable.filePath(QStringLiteral("resources/occt"))
  };
}

QString ViewportAssets::initializeOcct() const
{
  const QDir resources(occtResources);
  if (!resources.exists(QStringLiteral("Shaders")) || !resources.exists(QStringLiteral("XSTEPResource"))) {
    return QStringLiteral("OCCT runtime resources are missing: %1").arg(occtResources);
  }

  const std::pair<const char*, const char*> paths[] = {
    {"CSF_ShadersDirectory", "Shaders"}, {"CSF_MDTVTexturesDirectory", "Textures"},
    {"CSF_SHMessage", "SHMessage"}, {"CSF_XSMessage", "XSMessage"},
    {"CSF_STEPDefaults", "XSTEPResource"}, {"CSF_PluginDefaults", "StdResource"},
    {"CSF_StandardDefaults", "StdResource"}, {"CSF_XCAFDefaults", "StdResource"}
  };
  for (const auto& [variable, directory] : paths)
    qputenv(variable, resources.filePath(QLatin1String(directory)).toUtf8());
  try {
    Font_FontMgr::GetInstance()->InitFontDataBase();
    return {};
  } catch (const Standard_Failure& failure) {
    return QStringLiteral("Cannot initialize OCCT: %1").arg(QString::fromUtf8(failure.what()));
  }
}

} // namespace RoboCrap3D
