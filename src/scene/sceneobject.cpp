#include "sceneobject.h"
#include "3d/occcontroller.h"

#include <algorithm>
#include <cmath>

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

void SceneObject::setDiagnosticOverlays(bool showPoints, bool showNormals)
{
  if (m_showPoints == showPoints && m_showNormals == showNormals) return;
  m_showPoints = showPoints;
  m_showNormals = showNormals;
  emit diagnosticOverlaysChanged();
}

std::optional<RoboCrap3D::DrawingObject> SceneObject::drawing() const
{
  using namespace RoboCrap3D;
  DrawingObject result{objectId(), {}, visible(), drawingDiagnostics()};
  if (const auto* value = plane()) {
    const auto& origin = value->origin();
    const auto& normal = value->coefficients();
    const auto& axis = value->axisU();
    result.parts.append({DrawingPlane{{origin[0], origin[1], origin[2]},
        {normal[0], normal[1], normal[2]}, {axis[0], axis[1], axis[2]},
        value->width(), value->height()}, {}});
  } else if (const auto* value = cylinder()) {
    const auto& origin = value->origin();
    const auto& axis = value->axis();
    result.parts.append({DrawingCylinder{{origin[0], origin[1], origin[2]},
        {axis[0], axis[1], axis[2]}, value->radius(), value->length()}, {}});
  } else if (const auto* value = circle()) {
    result.parts.append({DrawingCurve{value->center(), value->normal(), V3d::UnitX(),
        value->radius(), value->radius()}, {{80, 157, 253}, 2.0}});
  } else if (const auto* value = edge()) {
    const auto& ellipse = value->ellipse();
    result.parts.append({DrawingCurve{ellipse.center, ellipse.majorAxis.cross(ellipse.minorAxis),
        ellipse.majorAxis, ellipse.majorRadius, ellipse.minorRadius}, {{0, 197, 197}, 2.0}});
  } else if (const auto* motion = machiningPath()) {
    for (std::size_t phase = 0; phase + 1 < motion->boundaries().size(); ++phase) {
      QList<V3d> positions;
      for (qsizetype i = motion->boundaries()[phase]; i <= motion->boundaries()[phase + 1]; ++i) {
        const V3d position = motion->points()[i].tcp.block<3, 1>(0, 3);
        if (positions.isEmpty() || (position - positions.back()).squaredNorm() > 1e-14)
          positions.append(position);
      }
      if (positions.size() < 2) continue; // A stationary HOME phase has no line.
      const std::array<int, 3> color = phase == 3 ? std::array<int, 3>{30, 130, 45}
          : phase == 0 || phase == 6 ? std::array<int, 3>{90, 65, 130} : std::array<int, 3>{0, 100, 210};
      result.parts.append({std::move(positions), {color, phase == 3 ? 3.0 : 2.0}});
    }
  } else return std::nullopt; // Metadata-only objects have no drawing.
  return result;
}

RoboCrap3D::DrawingDiagnostics SceneObject::drawingDiagnostics() const
{
  RoboCrap3D::DrawingDiagnostics result;
  const auto* plane = this->plane();
  const auto* cylinder = this->cylinder();
  const auto* circle = this->circle();
  const auto* edge = this->edge();
  for (const bool normals : {false, true}) {
    if (!(normals ? showNormals() : showPoints())) continue;
    const QVector<V3d>* samples = nullptr;
    QVector<V3d> edgeSamples;
    if (plane) samples = &plane->points();
    else if (cylinder) samples = &cylinder->points();
    else if (circle) samples = &circle->points();
    else if (edge) {
      const auto& ellipse = edge->ellipse();
      if (normals) edgeSamples.append(ellipse.center);
      else {
        constexpr int count = 64;
        const double fullTurn = 2.0 * std::acos(-1.0);
        for (int i = 0; i < count; ++i) {
          const double angle = fullTurn * i / count;
          edgeSamples.append(ellipse.center
              + ellipse.majorRadius * std::cos(angle) * ellipse.majorAxis
              + ellipse.minorRadius * std::sin(angle) * ellipse.minorAxis);
        }
      }
      samples = &edgeSamples;
    } else continue;
    if (samples->isEmpty()) continue;
    const qsizetype limit = normals ? 128 : 256;
    const qsizetype count = circle && normals ? 1 : samples->size();
    const qsizetype stride = 1 + (count - 1) / limit;
    const double radius = cylinder ? cylinder->radius()
        : circle ? circle->radius() : edge ? edge->ellipse().minorRadius : 0.0;
    const double length = plane
        ? std::clamp(0.05 * std::min(plane->width(), plane->height()), 1.0, 20.0)
        : std::clamp(0.25 * radius, 1.0, 20.0);
    for (qsizetype i = 0; i < count; i += stride) {
      const V3d& position = circle && normals ? circle->center() : (*samples)[i];
      if (!normals) { result.markers.append(position); continue; }
      V3d normal = V3d::Zero();
      if (plane) {
        const auto& coefficients = plane->coefficients();
        normal = {coefficients[0], coefficients[1], coefficients[2]};
      } else if (circle) normal = circle->normal();
      else if (edge) normal = edge->ellipse().majorAxis.cross(edge->ellipse().minorAxis).normalized();
      else {
        const auto& origin = cylinder->origin();
        const auto& axis = cylinder->axis();
        double axial = 0.0;
        for (int j = 0; j < 3; ++j) axial += (position[j] - origin[j]) * axis[j];
        for (int j = 0; j < 3; ++j) normal[j] = position[j] - origin[j] - axial * axis[j];
        const double norm2 = normal.squaredNorm();
        if (!std::isfinite(norm2) || norm2 <= 1e-18) continue;
        normal /= std::sqrt(norm2);
      }
      V3d tangent = std::abs(normal.x()) < 0.9 ? V3d::UnitX().eval() : V3d::UnitY().eval();
      tangent -= (std::abs(normal.x()) < 0.9 ? normal.x() : normal.y()) * normal;
      const double norm2 = tangent.squaredNorm();
      if (norm2 <= 1e-18) continue;
      tangent /= std::sqrt(norm2);
      const V3d tip = position + length * normal;
      result.normalLines.append({position, tip});
      result.normalLines.append({tip, tip - 0.25 * length * normal + 0.12 * length * tangent});
      result.normalLines.append({tip, tip - 0.25 * length * normal - 0.12 * length * tangent});
    }
  }
  return result;
}
