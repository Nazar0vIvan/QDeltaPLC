#include "scenemodel.h"

#include <utility>

#include <QJSEngine>

#include <array>
#include <cmath>
#include <algorithm>

namespace {

QString intersectionMessage(IntersectionStatus status)
{
  switch (status) {
    case IntersectionStatus::ParallelCylinderAxis:
      return SceneModel::tr("A plane parallel to the cylinder axis is not supported.");
    case IntersectionStatus::InvalidGeometry:
    case IntersectionStatus::Success:
      return SceneModel::tr("Cannot calculate a valid intersection for these surfaces.");
  }
  return SceneModel::tr("Cannot calculate a valid intersection for these surfaces.");
}

} // namespace

SceneModel::SceneModel(QObject* parent) : QObject(parent)
{}

QQmlListProperty<SceneObject> SceneModel::objects()
{
  // No append/clear/replace callbacks: QML cannot take over collection ownership.
  return {this, this, &SceneModel::objectCount, &SceneModel::objectAt};
}

qsizetype SceneModel::objectCount(QQmlListProperty<SceneObject>* list)
{
  return static_cast<SceneModel*>(list->data)->objectList().size();
}

SceneObject* SceneModel::objectAt(QQmlListProperty<SceneObject>* list, qsizetype index)
{
  return static_cast<SceneModel*>(list->data)->objectList().value(index);
}

SceneObject* SceneModel::findObject(quint32 objectId) const
{
  return m_objectsById.value(objectId, nullptr);
}

std::optional<std::array<const SceneObject*, 2>> SceneModel::intersectionPair(
    const QList<quint32>& ids) const
{
  if (ids.size() != 2 || ids[0] == ids[1]) return std::nullopt;
  const SceneObject* first = findObject(std::min(ids[0], ids[1]));
  const SceneObject* second = findObject(std::max(ids[0], ids[1]));
  if (!first || !second) return std::nullopt;
  // Keep the plane first regardless of selection order.
  if (!first->plane()) std::swap(first, second);
  if (!first->plane() || !second->cylinder()) return std::nullopt;
  return std::array<const SceneObject*, 2>{first, second};
}

void SceneModel::removeObjects(const QList<quint32>& ids)
{
  bool changed = false;
  for (quint32 id : ids) {
    SceneObject* object = m_objectsById.take(id);
    if (!object) continue;
    m_objects.removeOne(object);
    disconnect(object, nullptr, this, nullptr);
    emit objectRemoved(id);
    object->deleteLater();
    changed = true;
  }
  if (changed) emit objectsChanged();
}

bool SceneModel::canIntersect(const QList<quint32>& selectedIds) const
{
  return intersectionPair(selectedIds).has_value();
}

QList<quint32> SceneModel::intersect(const QList<quint32>& selectedIds)
{
  const auto pair = intersectionPair(selectedIds);
  if (!pair) {
    emit intersectionFailed(tr("Select one fitted plane and one fitted cylinder."));
    return {};
  }
  const SceneObject* first = (*pair)[0];
  const SceneObject* second = (*pair)[1];
  auto result = intersectSurfaces(*first->plane(), *second->cylinder());
  if (result.status != IntersectionStatus::Success || !result.edge) {
    emit intersectionFailed(intersectionMessage(result.status));
    return {};
  }
  const auto classification = first->classification() == second->classification()
      ? first->classification() : SceneObject::Unclassified;
  auto* object = createObject({tr("Edge %1").arg(m_nextEdgeNumber), SceneObject::Edge, classification, {}});
  if (!object) {
    emit intersectionFailed(tr("Cannot create an edge: scene object IDs are exhausted."));
    return {};
  }
  const std::array<quint32, 2> sources{std::min(first->objectId(), second->objectId()),
                                       std::max(first->objectId(), second->objectId())};
  object->setEdgeGeometry(std::move(*result.edge), sources);
  ++m_nextEdgeNumber;
  appendObject(object);
  return {object->objectId()};
}

SceneObject* SceneModel::createObject(SceneObject::Description description)
{
  if (m_nextId == 0) return nullptr;
  description.name = description.name.trimmed();
  return new SceneObject(m_nextId++, description, this);
}

SceneObject* SceneModel::addMachiningPath(SceneMachiningData data, const QString& name)
{
  if (!data.motion) return nullptr;
  const bool automaticName = name.trimmed().isEmpty();
  auto* object = createObject({automaticName ? tr("Machining Path %1").arg(m_nextMachiningPathNumber) : name,
                              SceneObject::MachiningPath, SceneObject::Unclassified, {}});
  if (!object) return nullptr;
  object->setMachiningPath(std::move(data));
  if (automaticName) ++m_nextMachiningPathNumber;
  appendObject(object);
  return object;
}

void SceneModel::appendObject(SceneObject* object)
{
  QJSEngine::setObjectOwnership(object, QJSEngine::CppOwnership);
  if (object->geometry()) QJSEngine::setObjectOwnership(object->geometry(), QJSEngine::CppOwnership);
  m_objects.append(object);
  m_objectsById.insert(object->objectId(), object);
  connect(object, &SceneObject::visibleChanged, this, &SceneModel::onObjectVisibilityChanged);
  connect(object, &SceneObject::diagnosticOverlaysChanged, this, &SceneModel::onObjectDiagnosticOverlaysChanged);
  emit objectAdded(object);
  emit objectsChanged();
}

SceneObject* SceneModel::addObject(const QString& name,
                                  SceneObject::Kind kind, SceneObject::Classification classification)
{
  const QString trimmedName = name.trimmed();
  if (trimmedName.isEmpty()
      || kind < SceneObject::Plane || kind > SceneObject::Circle
      || classification < SceneObject::Unclassified || classification > SceneObject::Precise) {
    return nullptr;
  }
  auto* object = createObject({trimmedName, kind, classification, {}});
  if (!object) return nullptr;
  appendObject(object);
  return object;
}

SceneObject* SceneModel::addPlane(const QUrl& sourceUrl, const QString& name,
                                 const QVariantList& coefficients)
{
  if (coefficients.size() != 4) return nullptr;

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

  auto* object = createObject({name.trimmed().isEmpty() ? tr("Plane") : name,
                               SceneObject::Plane, SceneObject::Rough, sourceUrl});
  if (!object) return nullptr;
  object->setPlaneGeometry(values);
  appendObject(object);
  return object;
}

// Each insertion takes source metadata, display name and validated geometry.
SceneObject* SceneModel::addBoundedPlane(const QUrl& sourceUrl, const QString& name,
                                        BoundedPlane plane)
{
  auto* object = createObject({name.trimmed().isEmpty() ? tr("Plane") : name,
                               SceneObject::Plane, SceneObject::Rough, sourceUrl});
  if (!object) return nullptr;
  object->setPlaneGeometry(std::move(plane));
  appendObject(object);
  return object;
}

SceneObject* SceneModel::addBoundedCylinder(const QUrl& sourceUrl, const QString& name,
                                           BoundedCylinder cylinder)
{
  auto* object = createObject({name.trimmed().isEmpty() ? tr("Cylinder") : name,
                               SceneObject::Cylinder, SceneObject::Rough, sourceUrl});
  if (!object) return nullptr;
  object->setCylinderGeometry(std::move(cylinder));
  appendObject(object);
  return object;
}

// Source metadata, display name and fitted geometry define a circle insertion.
SceneObject* SceneModel::addCircle(const QUrl& sourceUrl, const QString& name, Circle circle)
{
  auto* object = createObject({name.trimmed().isEmpty() ? tr("Circle") : name,
                               SceneObject::Circle, SceneObject::Rough, sourceUrl});
  if (!object) return nullptr;
  object->setCircleGeometry(std::move(circle));
  appendObject(object);
  return object;
}

void SceneModel::onObjectVisibilityChanged()
{
  emit objectVisibilityChanged(qobject_cast<SceneObject*>(sender()));
}

void SceneModel::onObjectDiagnosticOverlaysChanged()
{
  emit objectDiagnosticOverlaysChanged(qobject_cast<SceneObject*>(sender()));
}

bool SceneModel::setObjectDiagnosticOverlays(SceneObject* object, bool showPoints, bool showNormals)
{
  if (!object || m_objectsById.value(object->objectId()) != object) return false;
  object->setDiagnosticOverlays(showPoints, showNormals);
  return true;
}

bool SceneModel::renameObject(SceneObject* object, const QString& name)
{
  const QString trimmedName = name.trimmed();
  if (!object || m_objectsById.value(object->objectId()) != object || trimmedName.isEmpty()) return false;
  object->setName(trimmedName);
  return true;
}

bool SceneModel::setObjectVisible(SceneObject* object, bool visible)
{
  if (!object || m_objectsById.value(object->objectId()) != object) return false;
  object->setVisible(visible);
  return true;
}
