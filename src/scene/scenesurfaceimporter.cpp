#include "scenesurfaceimporter.h"

#include <utility>

#include "scenemodel.h"
#include "geometry/utils.h"

#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QJsonObject>

namespace {

enum class ReadError { None, Source, File, Json, Samples, CircleSamples };

struct ReadResult {
  std::optional<ProbeSamples> samples;
  QString name;
  ReadError error = ReadError::None;
};

ReadResult readSamples(const QUrl& source, bool circle)
{
  QString path;
  if (source.isLocalFile()) path = source.toLocalFile();
  else if (source.scheme() == QStringLiteral("qrc") && source.host().isEmpty())
    path = QStringLiteral(":") + source.path();
  if (!source.isValid() || path.isEmpty()) return {{}, {}, ReadError::Source};

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {{}, {}, ReadError::File};
  const QByteArray contents = file.readAll();
  if (file.error() != QFileDevice::NoError) return {{}, {}, ReadError::File};
  const QJsonDocument document = QJsonDocument::fromJson(contents);
  if (document.isNull()) return {{}, {}, ReadError::Json};
  if (circle) {
    const QJsonValue pointValue = document.isObject()
        ? document.object().value(QStringLiteral("points")) : QJsonValue(document.array());
    if (!pointValue.isArray()) return {{}, {}, ReadError::CircleSamples};
    auto points = jsonArrayToPoints(pointValue.toArray());
    if (!points) return {{}, {}, ReadError::CircleSamples};
    for (const V3d& point : std::as_const(*points))
      if (!point.allFinite()) return {{}, {}, ReadError::CircleSamples};
    // Circle samples are raw coordinates; no probe metadata is read or applied.
    return {ProbeSamples{std::move(*points)}, QFileInfo(path).completeBaseName(), ReadError::None};
  }
  auto samples = decodeProbeSamples(document);
  if (!samples) return {{}, {}, ReadError::Samples};
  return {std::move(samples), QFileInfo(path).completeBaseName(), ReadError::None};
}

QString readErrorText(ReadError error)
{
  switch (error) {
    case ReadError::Source: return SceneSurfaceImporter::tr("Choose a local JSON point file.");
    case ReadError::File: return SceneSurfaceImporter::tr("Cannot read the selected JSON file.");
    case ReadError::Json: return SceneSurfaceImporter::tr("The selected file is not valid JSON.");
    case ReadError::Samples: return SceneSurfaceImporter::tr(
        "Expected finite [x, y, z] points in an array, or an object with points, "
        "a finite nonnegative radius and dir equal to 1 or -1.");
    case ReadError::CircleSamples: return SceneSurfaceImporter::tr(
        "Expected finite [x, y, z] circle points in an array or an object with a points array.");
    case ReadError::None: return {};
  }
  return {};
}

} // namespace

SceneSurfaceImporter::SceneSurfaceImporter(QObject* parent) : QObject(parent)
{}

void SceneSurfaceImporter::load(const QUrl& sourceUrl, SceneModel* scene)
{
  importSurface(sourceUrl, scene, Surface::Plane);
}

void SceneSurfaceImporter::loadCylinder(const QUrl& sourceUrl, SceneModel* scene)
{
  importSurface(sourceUrl, scene, Surface::Cylinder);
}

void SceneSurfaceImporter::loadCircle(const QUrl& sourceUrl, SceneModel* scene)
{
  importSurface(sourceUrl, scene, Surface::Circle);
}

void SceneSurfaceImporter::importSurface(const QUrl& sourceUrl, SceneModel* scene, Surface surface)
{
  if (m_busy || !scene) {
    emit failed(m_busy ? tr("Another surface import is already in progress.")
                       : tr("Choose a destination scene."));
    return;
  }
  // Signal handlers may destroy the destination; no worker or event pumping is involved.
  QPointer<SceneModel> destination(scene);
  m_busy = true;
  emit busyChanged();
  ReadResult input = readSamples(sourceUrl, surface == Surface::Circle);
  QString error = readErrorText(input.error);
  QPointer<SceneObject> object;
  if (input.samples && destination) {
    if (surface == Surface::Plane) {
      auto plane = BoundedPlane::fromSamples(std::move(*input.samples));
      if (plane) object = destination->addBoundedPlane(sourceUrl, input.name, std::move(*plane));
      else error = tr("Could not fit a bounded plane from these samples.");
    } else if (surface == Surface::Cylinder) {
      auto cylinder = BoundedCylinder::fromSamples(std::move(*input.samples));
      if (cylinder) object = destination->addBoundedCylinder(sourceUrl, input.name, std::move(*cylinder));
      else error = tr("Could not fit a bounded cylinder with a positive compensated radius from these samples.");
    } else {
      auto circle = Circle::fromPoints(std::move(input.samples->points));
      if (circle) object = destination->addCircle(sourceUrl, input.name, std::move(*circle));
      else error = tr("Could not fit a circle. Supply at least three non-collinear points "
                      "that determine a finite circle.");
    }
  }
  if (!object && error.isEmpty()) error = tr("Could not insert the surface into the destination scene.");
  m_busy = false;
  emit busyChanged();
  if (object) emit loaded(object);
  else emit failed(error.isEmpty() ? tr("The destination object was closed during import.") : error);
}
