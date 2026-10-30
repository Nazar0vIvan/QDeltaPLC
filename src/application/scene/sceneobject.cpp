#include "sceneobject.h"
#include "viewport/occcontroller.h"

#include <QVariantMap>

#include <algorithm>
#include <cmath>

#include <utility>

namespace {

// Four decimals; magnitudes below half the last digit show as zero, never "-0.0000".
QString formatReal(double value)
{
  return QString::number(std::abs(value) < 0.00005 ? 0.0 : value, 'f', 4);
}

QString formatVector(const std::array<double, 3>& value)
{
  return QStringLiteral("[%1; %2; %3]")
      .arg(formatReal(value[0]), formatReal(value[1]), formatReal(value[2]));
}

QVariant propertyRow(const QString& label, const QString& value)
{
  return QVariantMap{{QStringLiteral("label"), label}, {QStringLiteral("value"), value}};
}

// A 1 px line between groups of rows.
QVariant separatorRow()
{
  return QVariantMap{{QStringLiteral("separator"), true}};
}

// A group heading with a check box bound to the named SceneObject visibility property.
QVariant toggleRow(const QString& label, const QString& property)
{
  return QVariantMap{{QStringLiteral("label"), label}, {QStringLiteral("toggle"), property}};
}

QString radiusDiameter(double radius)
{
  return QStringLiteral("%1 / %2").arg(formatReal(radius), formatReal(2 * radius));
}

// TCP positions of the points, without repeated positions.
QList<V3d> tcpPolyline(const QVector<ChamferJointPoint>& points)
{
  QList<V3d> positions;
  for (const auto& point : points) {
    const V3d position = point.tcp.block<3, 1>(0, 3);
    if (positions.isEmpty() || (position - positions.back()).squaredNorm() > 1e-14)
      positions.append(position);
  }
  return positions;
}

} // namespace

SceneObject::SceneObject(quint32 objectId, const Description& description, QObject* parent)
  : QObject(parent), m_objectId(objectId), m_name(description.name), m_kind(description.kind),
    m_classification(description.classification), m_sourceUrl(description.sourceUrl)
{}

QVariantList SceneObject::properties() const
{
  if (const auto* value = plane()) {
    const auto& normal = value->coefficients();
    return {propertyRow(tr("Normal"), formatVector({normal[0], normal[1], normal[2]})),
            propertyRow(tr("Origin"), formatVector(value->origin())),
            propertyRow(tr("Width"), formatReal(value->width())),
            propertyRow(tr("Height"), formatReal(value->height())),
            propertyRow(tr("Points"), QString::number(value->points().size())),
            propertyRow(tr("Offset (d)"), formatReal(normal[3]))};
  }
  if (const auto* value = cylinder()) {
    return {propertyRow(tr("Origin"), formatVector(value->origin())),
            propertyRow(tr("Axis"), formatVector(value->axis())),
            propertyRow(tr("Radius"), formatReal(value->radius())),
            propertyRow(tr("Diameter"), formatReal(2 * value->radius())),
            propertyRow(tr("Length"), formatReal(value->length())),
            propertyRow(tr("Points"), QString::number(value->points().size()))};
  }
  if (const auto* value = sphere()) {
    const V3d& center = value->center;
    return {propertyRow(tr("Nominal radius"), formatReal(value->radius)),
            propertyRow(tr("Nominal diameter"), formatReal(2 * value->radius)),
            separatorRow(),
            toggleRow(tr("Ball-center sphere"), QStringLiteral("showBallCenterSphere")),
            propertyRow(tr("Radius/Diameter"), radiusDiameter(value->ballCenterRadius)),
            separatorRow(),
            toggleRow(tr("Compensated sphere"), QStringLiteral("showCompensatedSphere")),
            propertyRow(tr("Radius/Diameter"), radiusDiameter(value->compensatedRadius)),
            separatorRow(),
            propertyRow(tr("Fitted center"), formatVector({center.x(), center.y(), center.z()})),
            propertyRow(tr("Fit RMS"), formatReal(value->rms)),
            propertyRow(tr("Points"), QString::number(value->roughPoints.size()))};
  }
  return {};
}

void SceneObject::setName(const QString& name)
{
  if (m_name == name) return;
  m_name = name;
  emit nameChanged();
}

void SceneObject::setMachiningPath(SceneMachiningData data)
{
  m_machiningPath = new SceneMachiningPath(std::move(data), this);
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

void SceneObject::setSphereDisplay(bool ballCenter, bool compensated)
{
  if (m_showBallCenterSphere == ballCenter && m_showCompensatedSphere == compensated) return;
  m_showBallCenterSphere = ballCenter;
  m_showCompensatedSphere = compensated;
  emit sphereDisplayChanged();
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
  } else if (const auto* value = edge()) {
    const auto& ellipse = value->edge.ellipse();
    result.parts.append({DrawingCurve{ellipse.center, ellipse.majorAxis.cross(ellipse.minorAxis),
        ellipse.majorAxis, ellipse.majorRadius, ellipse.minorRadius}, {{0, 197, 197}, 2.0}});
  } else if (const auto* value = sphere()) {
    result.parts.append({DrawingSphere{value->center, value->compensatedRadius}, {}, showCompensatedSphere()});
    // Translucent, so the compensated sphere inside it stays visible.
    result.parts.append({DrawingSphere{value->center, value->ballCenterRadius},
                         {{200, 200, 200}, 0.0, false, 0.8}, showBallCenterSphere()});
    // Decorative surroundings for the visual collision check: the table top and the stand.
    const CalibrationTable& table = value->table;
    const V3d tableCenter = value->center + V3d{0.5 * table.lengthX - table.sphereFromMinusX,
                                                0.5 * table.widthY - table.sphereFromMinusY, -table.sphereAboveTop};
    result.parts.append({DrawingPlane{tableCenter, V3d::UnitZ(), V3d::UnitX(), table.lengthX, table.widthY},
                         {{190, 190, 190}, 0.0, true}});
    result.parts.append({DrawingModel{value->standModel, value->center}, {{140, 140, 140}, 0.0, true}});
  } else if (const auto* motion = machiningPath()) {
    for (std::size_t phase = 0; phase + 1 < motion->boundaries().size(); ++phase) {
      const auto first = motion->boundaries()[phase];
      auto positions = tcpPolyline(motion->points().mid(first, motion->boundaries()[phase + 1] - first + 1));
      if (positions.size() < 2) continue; // A stationary HOME phase has no line.
      const std::array<int, 3> color = phase == 3 ? std::array<int, 3>{30, 130, 45}
          : phase == 0 || phase == 6 ? std::array<int, 3>{90, 65, 130} : std::array<int, 3>{0, 100, 210};
      result.parts.append({std::move(positions), {color, phase == 3 ? 3.0 : 2.0}});
    }
  } else if (const auto* value = touchPath()) {
    // Approach/Retract green, PTP purple, the other LIN transfers blue, as for machining paths.
    for (const auto& segment : value->motion->segments()) {
      auto positions = tcpPolyline(value->motion->points().mid(segment.first, segment.last - segment.first + 1));
      if (positions.size() < 2) continue;
      const ProbeSegment kind = segment.phase.segment;
      const bool touching = kind == ProbeSegment::Approach || kind == ProbeSegment::Retract;
      const bool ptp = kind == ProbeSegment::ToEntry || kind == ProbeSegment::ReturnHome;
      const std::array<int, 3> color = touching ? std::array<int, 3>{30, 130, 45}
          : ptp ? std::array<int, 3>{90, 65, 130} : std::array<int, 3>{0, 100, 210};
      result.parts.append({std::move(positions), {color, touching ? 3.0 : 2.0}});
    }
  } else return std::nullopt; // Metadata-only objects have no drawing.
  return result;
}

RoboCrap3D::DrawingDiagnostics SceneObject::drawingDiagnostics() const
{
  RoboCrap3D::DrawingDiagnostics result;
  const auto* plane = this->plane();
  const auto* cylinder = this->cylinder();
  const auto* sceneEdge = this->edge();
  const EdgeGeometry* edge = sceneEdge ? &sceneEdge->edge : nullptr;
  for (const bool normals : {false, true}) {
    if (!(normals ? showNormals() : showPoints())) continue;
    const QVector<V3d>* samples = nullptr;
    QVector<V3d> edgeSamples;
    if (plane) samples = &plane->points();
    else if (cylinder) samples = &cylinder->points();
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
    } else if (const auto* sphere = this->sphere(); sphere && !normals) samples = &sphere->roughPoints;
    else continue; // Sphere readings have no normal lines.
    if (samples->isEmpty()) continue;
    const qsizetype limit = normals ? 128 : 256;
    const qsizetype count = samples->size();
    const qsizetype stride = 1 + (count - 1) / limit;
    const double radius = cylinder ? cylinder->radius() : edge ? edge->ellipse().minorRadius : 0.0;
    const double length = plane
        ? std::clamp(0.05 * std::min(plane->width(), plane->height()), 1.0, 20.0)
        : std::clamp(0.25 * radius, 1.0, 20.0);
    for (qsizetype i = 0; i < count; i += stride) {
      const V3d& position = (*samples)[i];
      if (!normals) { result.markers.append(position); continue; }
      V3d normal = V3d::Zero();
      if (plane) {
        const auto& coefficients = plane->coefficients();
        normal = {coefficients[0], coefficients[1], coefficients[2]};
      } else if (edge) normal = edge->ellipse().majorAxis.cross(edge->ellipse().minorAxis).normalized();
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
