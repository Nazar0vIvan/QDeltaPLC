#include "planeimporter.h"

#include "plane.h"

#include <QFileInfo>
#include <QThread>

#include <exception>
#include <memory>

namespace {

struct ImportResult
{
  QVariantList coefficients;
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

void PlaneImporter::load(const QUrl& sourceUrl)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (busy()) return;

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
      const auto plane = Plane::fromJsonFile(path);
      if (!plane) {
        result->error = tr("Could not fit a plane from this file. Expected a JSON array of at least "
                           "three finite [x, y, z] points with non-collinear XY coordinates. "
                           "The current fitter does not support vertical planes.");
        return;
      }
      for (int i = 0; i < 4; ++i) result->coefficients.append(plane->coeffs[i]);
    } catch (const std::exception&) {
      result->error = tr("Plane import failed while reading or fitting the point data.");
    }
  });
  m_worker->setParent(this);
  connect(m_worker, &QThread::finished, this, [this, result, sourceUrl, path]() {
    m_worker->wait();
    m_worker->deleteLater();
    m_worker = nullptr;
    emit busyChanged();
    if (!result->error.isEmpty()) {
      emit failed(result->error);
    } else {
      emit loaded(sourceUrl, QFileInfo(path).completeBaseName(), result->coefficients);
    }
  }, Qt::QueuedConnection);
  m_worker->start();
  emit busyChanged();
}
