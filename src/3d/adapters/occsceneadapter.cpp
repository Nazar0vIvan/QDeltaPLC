#include "occsceneadapter.h"

#include "scene/scenemodel.h"
#include "scene/sceneobject.h"

#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Builder.hxx>
#include <gp_Ax3.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <QDebug>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>

#include <algorithm>
#include <cmath>
#include <optional>

namespace RoboCrap3D {

namespace {

std::optional<TopoDS_Shape> makeMotionPhase(const ChamferMotion& motion, std::size_t phase)
{
  BRepBuilderAPI_MakePolygon polygon;
  std::optional<gp_Pnt> previous;
  for (qsizetype i = motion.boundaries()[phase]; i <= motion.boundaries()[phase + 1]; ++i) {
    const auto& tcp = motion.points()[i].tcp;
    const gp_Pnt point(tcp(0, 3), tcp(1, 3), tcp(2, 3));
    if (previous && previous->SquareDistance(point) <= 1e-14) continue;
    polygon.Add(point);
    previous = point;
  }
  if (!polygon.IsDone()) return std::nullopt; // A stationary HOME phase has no line.
  return polygon.Shape();
}

std::optional<TopoDS_Face> makeBoundedPlaneFace(const BoundedPlane& plane)
{
  const gp_Pnt origin(plane.origin()[0], plane.origin()[1], plane.origin()[2]);
  const gp_Dir normal(plane.coefficients()[0], plane.coefficients()[1], plane.coefficients()[2]);
  const gp_Dir axisU(plane.axisU()[0], plane.axisU()[1], plane.axisU()[2]);
  const gp_Pln surface(gp_Ax3(origin, normal, axisU));
  BRepBuilderAPI_MakeFace builder(surface, -plane.width() / 2.0, plane.width() / 2.0,
                                 -plane.height() / 2.0, plane.height() / 2.0);
  if (!builder.IsDone()) return std::nullopt;
  const TopoDS_Face& face = builder.Face();
  if (face.IsNull()) return std::nullopt;
  return face;
}

std::optional<TopoDS_Face> makeBoundedCylinderFace(const BoundedCylinder& cylinder)
{
  const auto& center = cylinder.origin();
  const auto& direction = cylinder.axis();
  // Use a stable radial reference; gp_Ax3 derives the other tangent direction.
  const bool useX = std::abs(direction[0]) < 0.9;
  const double projection = useX ? direction[0] : direction[1];
  const gp_Dir radial(useX ? 1.0 - projection * direction[0] : -projection * direction[0],
                      useX ? -projection * direction[1] : 1.0 - projection * direction[1],
                      -projection * direction[2]);
  const gp_Ax3 frame(gp_Pnt(center[0], center[1], center[2]),
                     gp_Dir(direction[0], direction[1], direction[2]), radial);
  const gp_Cylinder surface(frame, cylinder.radius());
  const double fullTurn = 2.0 * std::acos(-1.0);
  BRepBuilderAPI_MakeFace builder(surface, 0.0, fullTurn,
                                 -cylinder.length() / 2.0, cylinder.length() / 2.0);
  if (!builder.IsDone()) return std::nullopt;
  const TopoDS_Face& face = builder.Face();
  if (face.IsNull()) return std::nullopt;
  return face;
}

std::optional<TopoDS_Edge> makeCircleEdge(const Circle& circle)
{
  const V3d& center = circle.center();
  const V3d& normal = circle.normal();
  const gp_Ax2 frame(gp_Pnt(center.x(), center.y(), center.z()),
                     gp_Dir(normal.x(), normal.y(), normal.z()));
  BRepBuilderAPI_MakeEdge builder(gp_Circ(frame, circle.radius()));
  if (!builder.IsDone() || builder.Edge().IsNull()) return std::nullopt;
  return builder.Edge();
}

std::optional<TopoDS_Edge> makeIntersectionEdge(const EdgeGeometry& edge)
{
  const auto& ellipse = edge.ellipse();
  const V3d normal = ellipse.majorAxis.cross(ellipse.minorAxis);
  const gp_Ax2 frame(gp_Pnt(ellipse.center.x(), ellipse.center.y(), ellipse.center.z()),
                     gp_Dir(normal.x(), normal.y(), normal.z()),
                     gp_Dir(ellipse.majorAxis.x(), ellipse.majorAxis.y(), ellipse.majorAxis.z()));
  // Full analytic curves preserve the numerical frame without sampling or trimming.
  if (ellipse.isCircle()) {
    BRepBuilderAPI_MakeEdge builder(gp_Circ(frame, ellipse.minorRadius));
    if (!builder.IsDone() || builder.Edge().IsNull()) return std::nullopt;
    return builder.Edge();
  }
  BRepBuilderAPI_MakeEdge builder(gp_Elips(frame, ellipse.majorRadius, ellipse.minorRadius));
  if (!builder.IsDone() || builder.Edge().IsNull()) return std::nullopt;
  return builder.Edge();
}

std::optional<TopoDS_Compound> makeDiagnosticShape(const SceneObject* object, bool normals)
{
  using Point = BoundedPlane::Point;
  const QVector<V3d>* samples = nullptr;
  const BoundedPlane* plane = object->plane();
  const BoundedCylinder* cylinder = object->cylinder();
  const Circle* circle = object->circle();
  if (plane) samples = &plane->points();
  else if (cylinder) samples = &cylinder->points();
  else if (circle) samples = &circle->points();
  else return std::nullopt;
  if (samples->empty()) return std::nullopt;

  constexpr qsizetype kMaxMarkers = 256;
  constexpr qsizetype kMaxNormals = 128;
  const qsizetype limit = normals ? kMaxNormals : kMaxMarkers;
  const qsizetype sampleCount = circle && normals ? 1 : samples->size();
  const qsizetype stride = 1 + (sampleCount - 1) / limit;
  const double length = plane
      ? std::clamp(0.05 * std::min(plane->width(), plane->height()), 1.0, 20.0)
      : std::clamp(0.25 * (cylinder ? cylinder->radius() : circle->radius()), 1.0, 20.0);
  BRep_Builder builder;
  TopoDS_Compound compound;
  builder.MakeCompound(compound);
  std::size_t count = 0;
  for (qsizetype i = 0; i < sampleCount; i += stride) {
    const V3d& point = circle && normals ? circle->center() : (*samples)[i];
    const gp_Pnt start(point[0], point[1], point[2]);
    if (!normals) {
      BRepBuilderAPI_MakeVertex vertex(start);
      builder.Add(compound, vertex.Vertex());
      ++count;
      continue;
    }

    Point direction{};
    if (plane) {
      direction = {plane->coefficients()[0], plane->coefficients()[1], plane->coefficients()[2]};
    } else if (circle) {
      direction = {circle->normal().x(), circle->normal().y(), circle->normal().z()};
    } else {
      double axial = 0.0;
      for (int j = 0; j < 3; ++j)
        axial += (point[j] - cylinder->origin()[j]) * cylinder->axis()[j];
      double norm2 = 0.0;
      for (int j = 0; j < 3; ++j) {
        direction[j] = point[j] - cylinder->origin()[j] - axial * cylinder->axis()[j];
        norm2 += direction[j] * direction[j];
      }
      if (!std::isfinite(norm2) || norm2 <= 1e-18) continue;
      const double norm = std::sqrt(norm2);
      for (double& value : direction) value /= norm;
    }
    const double projection = std::abs(direction[0]) < 0.9 ? direction[0] : direction[1];
    Point tangent{std::abs(direction[0]) < 0.9 ? 1.0 : 0.0,
                  std::abs(direction[0]) < 0.9 ? 0.0 : 1.0, 0.0};
    double tangentNorm2 = 0.0;
    for (int j = 0; j < 3; ++j) {
      tangent[j] -= projection * direction[j];
      tangentNorm2 += tangent[j] * tangent[j];
    }
    if (tangentNorm2 <= 1e-18) continue;
    const double tangentNorm = std::sqrt(tangentNorm2);
    for (double& value : tangent) value /= tangentNorm;

    Point tip{}, wingA{}, wingB{};
    for (int j = 0; j < 3; ++j) {
      tip[j] = point[j] + length * direction[j];
      wingA[j] = tip[j] - 0.25 * length * direction[j] + 0.12 * length * tangent[j];
      wingB[j] = tip[j] - 0.25 * length * direction[j] - 0.12 * length * tangent[j];
    }
    const gp_Pnt end(tip[0], tip[1], tip[2]);
    BRepBuilderAPI_MakeEdge stem(start, end);
    BRepBuilderAPI_MakeEdge left(end, gp_Pnt(wingA[0], wingA[1], wingA[2]));
    BRepBuilderAPI_MakeEdge right(end, gp_Pnt(wingB[0], wingB[1], wingB[2]));
    if (!stem.IsDone() || !left.IsDone() || !right.IsDone()) continue;
    builder.Add(compound, stem.Edge());
    builder.Add(compound, left.Edge());
    builder.Add(compound, right.Edge());
    ++count;
  }
  return count > 0 ? std::optional<TopoDS_Compound>{compound} : std::nullopt;
}

} // namespace

bool OccSceneAdapter::removeObject(quint32 objectId)
{
  const auto it = m_parts.find(objectId);
  if (it == m_parts.end()) return false;
  for (auto part : it->surfaces) (void)m_scene.removePart(part);
  if (it->points) (void)m_scene.removePart(*it->points);
  if (it->normals) (void)m_scene.removePart(*it->normals);
  m_parts.erase(it);
  return true;
}

bool OccSceneAdapter::synchronize(const SceneModel* applicationScene)
{
  // Full synchronization installs a scene; IDs are local to that scene.
  bool changed = !m_parts.isEmpty();
  for (const Presentation& part : m_parts) {
    for (auto id : part.surfaces) (void)m_scene.removePart(id);
    if (part.points) (void)m_scene.removePart(*part.points);
    if (part.normals) (void)m_scene.removePart(*part.normals);
  }
  m_parts.clear();
  if (applicationScene) {
    for (const SceneObject* object : applicationScene->objectList())
      changed = synchronizeObject(object) || changed;
  }
  return changed;
}

bool OccSceneAdapter::synchronizeObject(const SceneObject* object, bool overlaysChanged)
{
  if (!object) return false;
  auto it = m_parts.find(object->objectId());
  const bool created = it == m_parts.end();
  if (created && object->machiningPath()) {
    std::vector<OccScene::PartId> parts;
    for (std::size_t phase = 0; phase + 1 < object->machiningPath()->boundaries().size(); ++phase) {
      const auto shape = makeMotionPhase(*object->machiningPath(), phase);
      if (!shape) continue;
      OccPartProps props;
      props.color = phase == 3 ? rgb(30, 130, 45)
          : phase == 0 || phase == 6 ? rgb(90, 65, 130) : rgb(0, 100, 210);
      props.selectionMode = OccSelectionMode::PartOnly;
      props.wireframe = true;
      props.lineWidth = phase == 3 ? 3.0 : 2.0;
      const auto part = m_scene.addShapePartWithId(*shape, props);
      if (!part) {
        for (auto id : parts) (void)m_scene.removePart(id);
        return false;
      }
      parts.push_back(*part);
    }
    if (parts.empty()) return false;
    it = m_parts.insert(object->objectId(), Presentation{std::move(parts), true, {}, {}});
  } else if (created) {
    std::optional<TopoDS_Shape> shape;
    if (const BoundedPlane* plane = object->plane()) shape = makeBoundedPlaneFace(*plane);
    else if (const BoundedCylinder* cylinder = object->cylinder()) shape = makeBoundedCylinderFace(*cylinder);
    else if (const Circle* circle = object->circle()) shape = makeCircleEdge(*circle);
    else if (const EdgeGeometry* edge = object->edge()) shape = makeIntersectionEdge(*edge);
    else return false;
    if (!shape) {
      qWarning() << "Cannot create geometry for scene object:" << object->objectId();
      return false;
    }
    OccPartProps props;
    props.color = object->edge() ? rgb(0, 197, 197) : kRoughSurfaceColor;
    props.selectionMode = OccSelectionMode::All;
    if (object->circle() || object->edge()) {
      props.wireframe = true;
      props.lineWidth = 2.0;
    }
    const auto part = m_scene.addShapePartWithId(*shape, props);
    if (!part) return false;
    it = m_parts.insert(object->objectId(), Presentation{{*part}, true, {}, {}});
  }
  const bool visible = object->visible() && (!object->machiningPath() || m_showMachiningPaths);
  const bool visibilityChanged = it->visible != visible;
  if (!created && !visibilityChanged && !overlaysChanged) return false;
  if (visibilityChanged) {
    for (auto id : it->surfaces) (void)m_scene.setPartVisible(id, visible);
    it->visible = visible;
  }
  it->points = synchronizeOverlay(object, it->points, false);
  it->normals = synchronizeOverlay(object, it->normals, true);
  return true;
}

std::optional<OccScene::PartId> OccSceneAdapter::synchronizeOverlay(
    const SceneObject* object, std::optional<OccScene::PartId> partId, bool normals)
{
  const bool enabled = object->visible() && (normals ? m_showNormals : m_showPoints);
  if (partId) {
    (void)m_scene.setPartVisible(*partId, enabled);
    return partId;
  }
  if (!enabled) return std::nullopt;
  const auto shape = makeDiagnosticShape(object, normals);
  if (!shape) return std::nullopt;
  OccPartProps props;
  props.color = normals ? Quantity_Color(0.2, 0.9, 0.9, Quantity_TOC_RGB)
                        : Quantity_Color(1.0, 0.85, 0.15, Quantity_TOC_RGB);
  props.selectionMode = OccSelectionMode::None;
  props.wireframe = true;
  props.topmost = true;
  props.markerSize = normals ? 0.0 : 3.0;
  props.lineWidth = normals ? 2.0 : 0.0;
  return m_scene.addShapePartWithId(*shape, props);
}

bool OccSceneAdapter::setDiagnosticOverlays(bool showPoints, bool showNormals, const SceneModel* applicationScene)
{
  if (m_showPoints == showPoints && m_showNormals == showNormals) return false;
  m_showPoints = showPoints;
  m_showNormals = showNormals;
  bool changed = false;
  if (applicationScene) {
    for (const SceneObject* object : applicationScene->objectList())
      changed = synchronizeObject(object, true) || changed;
  }
  return changed;
}

void OccSceneAdapter::setSelectedObjects(const QList<quint32>& ids)
{
  std::vector<OccScene::PartId> parts;
  parts.reserve(static_cast<std::size_t>(ids.size()));
  for (quint32 id : ids) {
    const auto it = m_parts.constFind(id);
    if (it != m_parts.cend())
      for (auto part : it->surfaces)
        if (std::find(parts.cbegin(), parts.cend(), part) == parts.cend()) parts.push_back(part);
  }
  m_scene.selectParts(parts);
}

quint32 OccSceneAdapter::objectIdFor(const Handle(AIS_InteractiveObject)& picked) const
{
  if (picked.IsNull()) return {};
  for (auto it = m_parts.cbegin(); it != m_parts.cend(); ++it) {
    for (auto part : it->surfaces)
      if (m_scene.partHandle(part) == picked) return it.key();
  }
  return {}; // Background or non-application geometry clears UI selection.
}

bool OccSceneAdapter::setMachiningPathsVisible(bool visible, const SceneModel* scene)
{
  if (m_showMachiningPaths == visible) return false;
  m_showMachiningPaths = visible;
  bool changed = false;
  if (scene)
    for (const auto* object : scene->objectList())
      if (object->machiningPath()) changed = synchronizeObject(object) || changed;
  return changed;
}

bool OccSceneAdapter::setMachiningPreview(const std::optional<ChamferPathParameters>& parameters)
{
  if (parameters && m_previewParameters && m_axisPreview
      && parameters->edge.center == m_previewParameters->edge.center
      && parameters->edge.minorRadius == m_previewParameters->edge.minorRadius
      && parameters->cylinderAxis == m_previewParameters->cylinderAxis
      && parameters->flipAxis == m_previewParameters->flipAxis) return false;
  m_previewParameters = parameters;
  bool changed = m_axisPreview.has_value();
  if (m_axisPreview) (void)m_scene.removePart(*m_axisPreview);
  m_axisPreview.reset();
  if (!parameters) return changed;
  V3d direction = parameters->cylinderAxis;
  const double norm = direction.stableNorm();
  if (!direction.allFinite() || norm <= GeomConst::Eps) return changed;
  direction /= norm;
  if (parameters->flipAxis) direction = -direction;
  const V3d origin = parameters->edge.center;
  const double length = std::clamp(parameters->edge.minorRadius, 5.0, 40.0);
  const V3d tip = origin + length * direction;
  const V3d side = direction.unitOrthogonal();
  const std::array<V3d, 3> starts{origin, tip, tip};
  const std::array<V3d, 3> ends{tip, tip - 0.25 * length * direction + 0.12 * length * side,
                                   tip - 0.25 * length * direction - 0.12 * length * side};
  BRep_Builder builder;
  TopoDS_Compound shape;
  builder.MakeCompound(shape);
  for (std::size_t i = 0; i < starts.size(); ++i) {
    BRepBuilderAPI_MakeEdge edge(gp_Pnt(starts[i].x(), starts[i].y(), starts[i].z()),
                                 gp_Pnt(ends[i].x(), ends[i].y(), ends[i].z()));
    if (!edge.IsDone()) return changed;
    builder.Add(shape, edge.Edge());
  }
  OccPartProps props;
  props.color = rgb(180, 35, 125);
  props.wireframe = true;
  props.lineWidth = 3.0;
  props.selectionMode = OccSelectionMode::None;
  m_axisPreview = m_scene.addShapePartWithId(shape, props);
  if (m_axisPreview) m_scene.partHandle(*m_axisPreview)->SetInfiniteState(true);
  return changed || m_axisPreview.has_value();
}

} // namespace RoboCrap3D
