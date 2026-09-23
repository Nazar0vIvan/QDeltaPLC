#include "scenemodel.h"

#include "planegeometry.h"
#include "cylindergeometry.h"

#include <QJSEngine>
#include <QThread>

#include <array>
#include <cmath>

SceneModel::SceneModel(QObject* parent) : QObject(parent)
{}

QQmlListProperty<SceneObject> SceneModel::objects()
{
  Q_ASSERT(QThread::currentThread() == thread());
  // No append/clear/replace callbacks: QML cannot take over collection ownership.
  return {this, this, &SceneModel::objectCount, &SceneModel::objectAt};
}

qsizetype SceneModel::objectCount(QQmlListProperty<SceneObject>* list)
{
  return static_cast<SceneModel*>(list->data)->m_objects.size();
}

SceneObject* SceneModel::objectAt(QQmlListProperty<SceneObject>* list, qsizetype index)
{
  return static_cast<SceneModel*>(list->data)->m_objects.value(index);
}

SceneObject* SceneModel::findObject(const QString& objectId) const
{
  Q_ASSERT(QThread::currentThread() == thread());
  for (SceneObject* object : m_objects) {
    if (object->objectId() == objectId) return object;
  }
  return nullptr;
}

void SceneModel::appendObject(SceneObject* object)
{
  Q_ASSERT(object->parent() == this);
  QJSEngine::setObjectOwnership(object, QJSEngine::CppOwnership);
  m_objects.append(object);
  emit objectsChanged();
}

SceneObject* SceneModel::addObject(const QString& objectId, const QString& name,
                                  SceneObject::Kind kind, SceneObject::Classification classification)
{
  Q_ASSERT(QThread::currentThread() == thread());
  const QString trimmedName = name.trimmed();
  if (objectId.trimmed().isEmpty() || findObject(objectId) || trimmedName.isEmpty()
      || kind < SceneObject::Plane || kind > SceneObject::MachiningPath
      || classification < SceneObject::Unclassified || classification > SceneObject::Precise) {
    return nullptr;
  }
  auto* object = new SceneObject(objectId, trimmedName, kind, classification, {}, this);
  appendObject(object);
  return object;
}

SceneObject* SceneModel::addPlane(const QUrl& sourceUrl, const QString& name,
                                 const QVariantList& coefficients)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (coefficients.size() != 4 || sourceUrl.isEmpty() || !sourceUrl.isValid()) return nullptr;

  std::array<double, 4> values{};
  for (int i = 0; i < 4; ++i) {
    const int type = coefficients[i].metaType().id();
    if (type != QMetaType::Double && type != QMetaType::Float
        && type != QMetaType::Int && type != QMetaType::UInt
        && type != QMetaType::LongLong && type != QMetaType::ULongLong) return nullptr;
    values[i] = coefficients[i].toDouble();
    if (!std::isfinite(values[i])) return nullptr;
  }
  const double norm = std::hypot(values[0], values[1], values[2]);
  if (std::abs(norm - 1.0) > 1e-6) return nullptr;

  return createPlane(sourceUrl, name, values);
}

SceneObject* SceneModel::addBoundedPlane(const QUrl& sourceUrl, const QString& name,
                                        std::shared_ptr<const BoundedPlane> plane)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!plane || sourceUrl.isEmpty() || !sourceUrl.isValid()) return nullptr;
  return createPlane(sourceUrl, name, plane->coefficients, plane);
}

SceneObject* SceneModel::addBoundedCylinder(const QUrl& sourceUrl, const QString& name,
                                           std::shared_ptr<const BoundedCylinder> cylinder)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!cylinder || sourceUrl.isEmpty() || !sourceUrl.isValid()) return nullptr;
  QString objectId;
  do {
    objectId = QStringLiteral("imported-cylinder-%1").arg(m_nextCylinderId++);
  } while (findObject(objectId));

  const QString trimmedName = name.trimmed();
  auto* object = new SceneObject(objectId, trimmedName.isEmpty() ? tr("Cylinder") : trimmedName,
                                 SceneObject::Cylinder, SceneObject::Rough, sourceUrl, this);
  object->m_geometry = new CylinderGeometry(std::move(cylinder), object);
  QJSEngine::setObjectOwnership(object->m_geometry, QJSEngine::CppOwnership);
  appendObject(object);
  return object;
}

SceneObject* SceneModel::createPlane(const QUrl& sourceUrl, const QString& name,
                                    const std::array<double, 4>& coefficients,
                                    std::shared_ptr<const BoundedPlane> bounded)
{
  QString objectId;
  do {
    objectId = QStringLiteral("imported-plane-%1").arg(m_nextPlaneId++);
  } while (findObject(objectId));

  const QString trimmedName = name.trimmed();
  auto* object = new SceneObject(objectId, trimmedName.isEmpty() ? tr("Plane") : trimmedName,
                                 SceneObject::Plane, SceneObject::Rough, sourceUrl, this);
  object->m_geometry = bounded ? new PlaneGeometry(std::move(bounded), object)
                              : new PlaneGeometry(coefficients, object);
  QJSEngine::setObjectOwnership(object->m_geometry, QJSEngine::CppOwnership);
  appendObject(object);
  return object;
}

bool SceneModel::renameObject(SceneObject* object, const QString& name)
{
  Q_ASSERT(QThread::currentThread() == thread());
  const QString trimmedName = name.trimmed();
  if (!object || !m_objects.contains(object) || trimmedName.isEmpty()) return false;
  object->setName(trimmedName);
  return true;
}

bool SceneModel::setObjectVisible(SceneObject* object, bool visible)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!object || !m_objects.contains(object)) return false;
  object->setVisible(visible);
  return true;
}
