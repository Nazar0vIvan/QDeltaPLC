#include "occcontroller.h"

#include "occt/cadloadworker.h"
#include "occt/occviewwindow.h"
#include "robot/robotpreviewstate.h"
#include "viewportassets.h"

#include <QJSEngine>
#include <QThread>
#include <QTimer>

#include <Standard_Failure.hxx>

#include <cmath>
#include <optional>

namespace RoboCrap3D {

namespace {

QVariantList toList(const V6d& values)
{
  QVariantList result;
  result.reserve(6);
  for (double value : values) result.append(value);
  return result;
}

std::optional<V6d> readValues(const QVariantList& values)
{
  if (values.size() != 6) return std::nullopt;
  V6d result{};
  for (int i = 0; i < 6; ++i) {
    const int type = values[i].metaType().id();
    if (type != QMetaType::Double && type != QMetaType::Float
        && type != QMetaType::Int && type != QMetaType::UInt
        && type != QMetaType::LongLong && type != QMetaType::ULongLong) return std::nullopt;
    bool ok = false;
    result[i] = values[i].toDouble(&ok);
    if (!ok || !std::isfinite(result[i])) return std::nullopt;
  }
  return result;
}

} // namespace

OccController::OccController(QObject* parent) : QObject(parent)
{
  createWindow();
}

OccController::~OccController()
{
  m_shuttingDown = true;
  ++m_generation;
  if (m_worker) {
    m_worker->requestInterruption();
    m_worker->wait();
  }
  if (m_window) {
    detachViewWindow(m_window);
    delete m_window.data();
  }
}

QWindow* OccController::viewWindow() const
{
  return m_window.data();
}

bool OccController::isReady() const
{
  return !m_loading && m_window && m_window->isReady();
}

QVariantList OccController::jointAngles() const
{
  return m_state ? toList(m_state->pose().joints) : QVariantList{};
}

QVariantList OccController::flangePose() const
{
  return m_state ? toList(m_state->pose().flange) : QVariantList{};
}

void OccController::createWindow()
{
  if (m_window || m_shuttingDown) return;
  m_window = new OccViewWindow();
  QJSEngine::setObjectOwnership(m_window, QJSEngine::CppOwnership);
  QObject::connect(m_window, &OccViewWindow::readyChanged, this, &OccController::readyChanged);
  QObject::connect(m_window, &OccViewWindow::errorOccurred, this, &OccController::setError);
  QObject::connect(m_window, &QObject::destroyed, this, [this]() {
    if (m_shuttingDown) return;
    emit readyChanged();
    // A host may destroy its child QWindow before QML's detach handler runs.
    // QPointer prevents double deletion; the persistent robot state is retained.
    QTimer::singleShot(0, this, [this]() { createWindow(); });
  });
  if (m_state && m_shapes) m_window->setScene(m_state, m_shapes);
  emit viewWindowChanged();
}

void OccController::detachViewWindow(QWindow* window)
{
  if (!m_window || window != m_window.data()) return;
  m_window->hide();
  m_window->releaseSurface();
  m_window->setParent(nullptr);
}

void OccController::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  emit errorStringChanged();
  if (!error.isEmpty()) emit message(error, true);
}

void OccController::loadRobot()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_loading || m_shuttingDown) return;
  if (m_state && m_shapes) {
    setError({});
    createWindow();
    m_window->setScene(m_state, m_shapes);
    return;
  }

  const ViewportAssets assets = ViewportAssets::applicationAssets();
  QString error;
  if (!m_occtInitialized) {
    m_occtInitialized = assets.initializeOcct(error);
    if (!m_occtInitialized) {
      setError(error);
      return;
    }
  }

  std::shared_ptr<RobotPreviewState> pending;
  try {
    const auto model = Kr10Model::fromJson(assets.modelFile);
    if (!model) {
      setError(QStringLiteral("Cannot read the robot definition: %1").arg(assets.modelFile));
      return;
    }
    pending = std::make_shared<RobotPreviewState>(*model);
    if (!pending->initialize(error)) {
      setError(error);
      return;
    }
  } catch (const Standard_Failure& failure) {
    setError(QStringLiteral("Invalid robot definition: %1").arg(QString::fromUtf8(failure.what())));
    return;
  }

  setError({});
  m_loading = true;
  emit loadingChanged();
  emit readyChanged();
  const quint64 generation = ++m_generation;
  m_worker = std::make_unique<CadLoadWorker>(pending->model(), assets);
  QObject::connect(m_worker.get(), &QThread::finished, this, [this, generation, pending]() {
    if (m_shuttingDown || generation != m_generation) return;
    m_worker->wait();
    auto result = std::make_shared<CadLoadResult>(m_worker->takeResult());
    m_worker.reset();
    m_loading = false;
    emit loadingChanged();
    if (!result->error.isEmpty()) {
      setError(result->error);
      emit readyChanged();
      return;
    }
    m_state = pending;
    m_shapes = result;
    m_warning = result->warning;
    emit warningStringChanged();
    if (!m_warning.isEmpty()) emit message(m_warning, false);
    createWindow();
    m_window->setScene(m_state, m_shapes);
    emit poseChanged();
    emit readyChanged();
  }, Qt::QueuedConnection);
  m_worker->start();
}

bool OccController::applyPose(const RobotPose& pose)
{
  if (!isReady() || !m_state) return false;
  try {
    if (!m_window->applyPose(pose)) {
      m_window->setScene(m_state, m_shapes);
      return false;
    }
    m_state->commit(pose);
    emit poseChanged();
    return true;
  } catch (const Standard_Failure&) {
    // Recreate presentation from the last committed state after a partial update.
    m_window->setScene(m_state, m_shapes);
    return false;
  }
}

QVariantList OccController::solve(const QVariantList& values, bool inverse)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!isReady() || !m_state) {
    setError(QStringLiteral("The robot preview is not ready."));
    return {};
  }
  const auto input = readValues(values);
  if (!input) {
    setError(QStringLiteral("Enter exactly six finite numeric values."));
    return {};
  }
  try {
    QString error;
    const auto pose = inverse ? m_state->inverse(*input, error) : m_state->forward(*input, error);
    if (!pose) {
      setError(error);
      return {};
    }
    if (!applyPose(*pose)) {
      setError(QStringLiteral("Cannot update the robot presentation."));
      return {};
    }
    setError({});
    return toList(inverse ? pose->joints : pose->flange);
  } catch (const Standard_Failure& failure) {
    setError(QStringLiteral("Kinematics failed: %1").arg(QString::fromUtf8(failure.what())));
    return {};
  }
}

QVariantList OccController::solveFK(const QVariantList& joints)
{
  return solve(joints, false);
}

QVariantList OccController::solveIK(const QVariantList& flange)
{
  return solve(flange, true);
}

} // namespace RoboCrap3D
