#include "sceneobject.h"

#include <utility>

// Identity, description and QObject ownership are the three constructor responsibilities.
SceneObject::SceneObject(quint32 objectId, const Description& description, QObject* parent)
  : QObject(parent), m_objectId(objectId), m_name(description.name), m_kind(description.kind),
    m_classification(description.classification), m_sourceUrl(description.sourceUrl)
{}

void SceneObject::setPlaneGeometry(BoundedPlane plane)
{
  m_geometry = new ScenePlaneGeometry(std::move(plane), this);
}

void SceneObject::setPlaneGeometry(const std::array<double, 4>& coefficients)
{
  m_geometry = new ScenePlaneGeometry(coefficients, this);
}

void SceneObject::setCylinderGeometry(BoundedCylinder cylinder)
{
  m_geometry = new SceneCylinderGeometry(std::move(cylinder), this);
}

const BoundedPlane* SceneObject::plane() const
{
  const auto* geometry = qobject_cast<const ScenePlaneGeometry*>(m_geometry);
  return geometry ? geometry->boundedPlane() : nullptr;
}

const BoundedCylinder* SceneObject::cylinder() const
{
  const auto* geometry = qobject_cast<const SceneCylinderGeometry*>(m_geometry);
  return geometry ? &geometry->boundedCylinder() : nullptr;
}

void SceneObject::setCircleGeometry(::Circle circle)
{
  m_geometry = new SceneCircleGeometry(std::move(circle), this);
}

const ::Circle* SceneObject::circle() const
{
  const auto* geometry = qobject_cast<const SceneCircleGeometry*>(m_geometry);
  return geometry ? &geometry->circle() : nullptr;
}

void SceneObject::setEdgeGeometry(EdgeGeometry edge, const std::array<quint32, 2>& sources)
{
  m_geometry = new SceneEdgeGeometry(std::move(edge), sources, this);
}

const EdgeGeometry* SceneObject::edge() const
{
  const auto* geometry = qobject_cast<const SceneEdgeGeometry*>(m_geometry);
  return geometry ? &geometry->edge() : nullptr;
}

void SceneObject::setName(const QString& name)
{
  if (m_name == name) return;
  m_name = name;
  emit nameChanged();
}

void SceneObject::setMachiningPath(SceneMachiningData data)
{
  m_geometry = new SceneMachiningPath(std::move(data), this);
}

const ChamferMotion* SceneObject::machiningPath() const
{
  const auto* geometry = qobject_cast<const SceneMachiningPath*>(m_geometry);
  return geometry ? &geometry->motion() : nullptr;
}

void SceneObject::setVisible(bool visible)
{
  if (m_visible == visible) return;
  m_visible = visible;
  emit visibleChanged();
}
