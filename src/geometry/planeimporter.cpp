#include "planeimporter.h"

#include "boundedplane.h"
#include "boundedcylinder.h"

#include <QFileInfo>
#include <QThread>
#include <QPointer>

#include <exception>
#include <functional>
#include <memory>
#include <utility>

struct PlaneImporter::ImportResult
{
  std::shared_ptr<const BoundedPlane> plane;
  std::shared_ptr<const BoundedCylinder> cylinder;
  QString error;
};

PlaneImporter::PlaneImporter(QObject* parent) : QObject(parent)
{}

PlaneImporter::~PlaneImporter()
{
  // The existing fitter is synchronous and cannot be interrupted mid-fit.
  if (m_worker) m_worker->wait();
}

void PlaneImporter::load(const QUrl& sourceUrl, SceneModel* scene)
{
  startImport(sourceUrl, scene, Surface::Plane);
}

void PlaneImporter::loadCylinder(const QUrl& sourceUrl, SceneModel* scene)
{
  startImport(sourceUrl, scene, Surface::Cylinder);
}

void PlaneImporter::startImport(const QUrl& sourceUrl, SceneModel* scene, Surface surface)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (busy()) {
    emit failed(tr("Another surface import is already in progress."));
    return;
  }
  if (!scene || scene->thread() != thread()) {
    emit failed(tr("Choose a scene on the application thread."));
    return;
  }

  QString path;
  if (sourceUrl.isLocalFile()) {
    path = sourceUrl.toLocalFile();
  } else if (sourceUrl.scheme() == QStringLiteral("qrc") && sourceUrl.host().isEmpty()) {
    path = QStringLiteral(":") + sourceUrl.path();
  }
  if (path.isEmpty() || !sourceUrl.isValid()) {
    emit failed(tr("Choose a local JSON point file."));
    return;
  }

  auto result = std::make_shared<ImportResult>();
  m_worker = QThread::create(&PlaneImporter::fitSurface, path, result, surface);
  m_worker->setParent(this);
  const QPointer<SceneModel> destination(scene);
  connect(m_worker, &QThread::finished, this,
          std::bind(&PlaneImporter::finishImport, this, result, sourceUrl, path, destination, surface),
          Qt::QueuedConnection);
  m_worker->start();
  emit busyChanged();
}

void PlaneImporter::fitSurface(const QString& path, std::shared_ptr<ImportResult> result, Surface surface)
{
  try {
    const QFileInfo file(path);
    if (!file.isFile() || !file.isReadable()) {
      result->error = tr("Cannot read the selected JSON file: %1").arg(path);
      return;
    }
    if (surface == Surface::Plane) {
      auto plane = BoundedPlane::fromJsonFile(path);
      if (!plane) {
        result->error = tr("Could not fit a plane from this file. Expected a JSON array of at least "
                           "three finite [x, y, z] points with non-collinear XY coordinates. "
                           "Projected bounds must have nonzero width and height. "
                           "The current fitter does not support vertical planes.");
        return;
      }
      result->plane = std::make_shared<const BoundedPlane>(std::move(*plane));
    } else {
      auto cylinder = BoundedCylinder::fromJsonFile(path);
      if (!cylinder) {
        result->error = tr("Could not fit a cylinder from this file. Expected a JSON array of at least "
                           "six finite [x, y, z] surface points spanning the curved side and axis. "
                           "The fit must have a positive radius and length with a low radial error.");
        return;
      }
      result->cylinder = std::make_shared<const BoundedCylinder>(std::move(*cylinder));
    }
  } catch (const std::exception&) {
    result->error = surface == Surface::Plane
        ? tr("Plane import failed while reading or fitting the point data.")
        : tr("Cylinder import failed while reading or fitting the point data.");
  }
}

void PlaneImporter::finishImport(std::shared_ptr<ImportResult> result, const QUrl& sourceUrl,
                                 const QString& path, QPointer<SceneModel> destination, Surface surface)
{
  m_worker->wait();
  if (!result->error.isEmpty()) {
    emit failed(result->error);
  } else if (!destination) {
    emit failed(tr("The destination scene was closed before import finished."));
  } else {
    SceneObject* object = surface == Surface::Plane
        ? destination->addBoundedPlane(sourceUrl, QFileInfo(path).completeBaseName(), result->plane)
        : destination->addBoundedCylinder(sourceUrl, QFileInfo(path).completeBaseName(), result->cylinder);
    if (object)
      emit loaded(object);
    else
      emit failed(surface == Surface::Plane
                  ? tr("Could not add the imported plane to the scene.")
                  : tr("Could not add the imported cylinder to the scene."));
  }
  m_worker->deleteLater();
  m_worker = nullptr;
  emit busyChanged();
}
