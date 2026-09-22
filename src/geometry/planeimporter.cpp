#include "planeimporter.h"

#include "boundedplane.h"

#include <QFileInfo>
#include <QThread>
#include <QPointer>

#include <exception>
#include <memory>
#include <utility>

namespace {

struct ImportResult
{
  std::shared_ptr<const BoundedPlane> plane;
  QString error;
};

} // namespace

PlaneImporter::PlaneImporter(QObject* parent) : QObject(parent)
{}

PlaneImporter::~PlaneImporter()
{
  // The existing fitter is synchronous and cannot be interrupted mid-fit.
  if (m_worker) m_worker->wait();
}

void PlaneImporter::load(const QUrl& sourceUrl, SceneModel* scene)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (busy()) return;
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
  m_worker = QThread::create([path, result]() {
    try {
      const QFileInfo file(path);
      if (!file.isFile() || !file.isReadable()) {
        result->error = tr("Cannot read the selected JSON file: %1").arg(path);
        return;
      }
      auto plane = BoundedPlane::fromJsonFile(path);
      if (!plane) {
        result->error = tr("Could not fit a plane from this file. Expected a JSON array of at least "
                           "three finite [x, y, z] points with non-collinear XY coordinates. "
                           "Projected bounds must have nonzero width and height. "
                           "The current fitter does not support vertical planes.");
        return;
      }
      result->plane = std::make_shared<const BoundedPlane>(std::move(*plane));
    } catch (const std::exception&) {
      result->error = tr("Plane import failed while reading or fitting the point data.");
    }
  });
  m_worker->setParent(this);
  const QPointer<SceneModel> destination(scene);
  connect(m_worker, &QThread::finished, this, [this, result, sourceUrl, path, destination]() {
    m_worker->wait();
    if (!result->error.isEmpty()) {
      emit failed(result->error);
    } else if (!destination) {
      emit failed(tr("The destination scene was closed before import finished."));
    } else {
      auto* object = destination->addBoundedPlane(sourceUrl, QFileInfo(path).completeBaseName(), result->plane);
      if (object)
        emit loaded(object);
      else
        emit failed(tr("Could not add the imported plane to the scene."));
    }
    m_worker->deleteLater();
    m_worker = nullptr;
    emit busyChanged();
  }, Qt::QueuedConnection);
  m_worker->start();
  emit busyChanged();
}
