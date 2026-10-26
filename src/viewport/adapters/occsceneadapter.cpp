#include "occsceneadapter.h"

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
#include <gp_Elips.hxx>
#include <gp_Pln.hxx>
#include <gp_Sphere.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <Standard_Failure.hxx>
#include <QStringList>
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace RoboCrap3D {
namespace {

gp_Pnt toOccPoint(const V3d& value)
{
  return {value.x(), value.y(), value.z()};
}

gp_Dir toOccDirection(const V3d& value)
{
  return {value.x(), value.y(), value.z()};
}

std::optional<TopoDS_Shape> makeShape(const DrawingPart& part)
{
  if (const auto* plane = std::get_if<DrawingPlane>(&part.geometry)) {
    const gp_Pln surface(gp_Ax3(toOccPoint(plane->origin), toOccDirection(plane->normal), toOccDirection(plane->axisU)));
    BRepBuilderAPI_MakeFace builder(surface, -plane->width / 2.0, plane->width / 2.0,
                                    -plane->height / 2.0, plane->height / 2.0);
    if (builder.IsDone() && !builder.Face().IsNull()) return builder.Face();
  } else if (const auto* cylinder = std::get_if<DrawingCylinder>(&part.geometry)) {
    const auto& axis = cylinder->axis;
    const bool useX = std::abs(axis.x()) < 0.9;
    const V3d reference = useX ? V3d::UnitX().eval() : V3d::UnitY().eval();
    const V3d radial = reference - (useX ? axis.x() : axis.y()) * axis;
    const gp_Cylinder surface(gp_Ax3(toOccPoint(cylinder->origin), toOccDirection(axis), toOccDirection(radial)),
                              cylinder->radius);
    BRepBuilderAPI_MakeFace builder(surface, 0.0, 2.0 * std::acos(-1.0),
                                    -cylinder->length / 2.0, cylinder->length / 2.0);
    if (builder.IsDone() && !builder.Face().IsNull()) return builder.Face();
  } else if (const auto* curve = std::get_if<DrawingCurve>(&part.geometry)) {
    const bool circle = curve->majorRadius == curve->minorRadius;
    const gp_Ax2 frame = circle ? gp_Ax2(toOccPoint(curve->center), toOccDirection(curve->normal))
        : gp_Ax2(toOccPoint(curve->center), toOccDirection(curve->normal), toOccDirection(curve->majorAxis));
    if (circle) {
      BRepBuilderAPI_MakeEdge builder(gp_Circ(frame, curve->minorRadius));
      if (builder.IsDone() && !builder.Edge().IsNull()) return builder.Edge();
    } else {
      BRepBuilderAPI_MakeEdge builder(gp_Elips(frame, curve->majorRadius, curve->minorRadius));
      if (builder.IsDone() && !builder.Edge().IsNull()) return builder.Edge();
    }
  } else if (const auto* sphere = std::get_if<DrawingSphere>(&part.geometry)) {
    BRepBuilderAPI_MakeFace builder(gp_Sphere(gp_Ax3(toOccPoint(sphere->center), gp_Dir(0.0, 0.0, 1.0)),
                                              sphere->radius));
    if (builder.IsDone() && !builder.Face().IsNull()) return builder.Face();
  } else if (const auto* positions = std::get_if<QList<V3d>>(&part.geometry)) {
    BRepBuilderAPI_MakePolygon polygon;
    for (const auto& position : *positions) polygon.Add(toOccPoint(position));
    if (polygon.IsDone()) return polygon.Shape();
  } else if (const auto* lines = std::get_if<QList<std::array<V3d, 2>>>(&part.geometry)) {
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    for (const auto& line : *lines) {
      BRepBuilderAPI_MakeEdge edge(toOccPoint(line[0]), toOccPoint(line[1]));
      if (!edge.IsDone()) return std::nullopt;
      builder.Add(compound, edge.Edge());
    }
    return compound;
  }
  return std::nullopt;
}

std::optional<TopoDS_Shape> markerShape(const QList<V3d>& markers)
{
  if (markers.isEmpty()) return std::nullopt;
  BRep_Builder builder;
  TopoDS_Compound compound;
  builder.MakeCompound(compound);
  for (const auto& position : markers)
    builder.Add(compound, BRepBuilderAPI_MakeVertex(toOccPoint(position)).Vertex());
  return compound;
}

} // namespace

QString OccSceneAdapter::removeParts(const std::vector<OccScene::PartId>& surfaces,
                                    const std::vector<OccScene::PartId>& diagnostics)
{
  QStringList errors;
  for (const auto* ids : {&surfaces, &diagnostics}) {
    for (const auto id : *ids) {
      try {
        if (!m_scene.removePart(id)) errors.append(QStringLiteral("cannot remove part %1").arg(id));
      } catch (const Standard_Failure& failure) {
        errors.append(QStringLiteral("part %1: %2").arg(id).arg(QString::fromUtf8(failure.what())));
      }
    }
  }
  return errors.join(QStringLiteral("; "));
}

std::pair<OccSceneAdapter::Presentation, QString> OccSceneAdapter::createPresentation(
    const DrawingObject& object)
{
  Presentation result;
  result.visible = object.visible;
  QString error;
  try {
    QList<std::pair<TopoDS_Shape, OccPartProps>> shapes;
    for (const auto& part : object.parts) {
      if (!part.visible) continue;
      const auto shape = makeShape(part);
      if (!shape) return {{}, QStringLiteral("Cannot construct drawing geometry.")};
      OccPartProps props;
      props.color = rgb(part.style.color[0], part.style.color[1], part.style.color[2]);
      props.lineWidth = part.style.lineWidth;
      props.transparency = part.style.transparency;
      props.wireframe = props.lineWidth > 0.0;
      // A sphere face has only its seam meridian and degenerate pole edges.
      props.faceBoundary = !std::holds_alternative<DrawingSphere>(part.geometry);
      props.selectionMode = part.style.decoration ? OccSelectionMode::None
          : std::holds_alternative<QList<V3d>>(part.geometry) ? OccSelectionMode::PartOnly : OccSelectionMode::All;
      shapes.append({*shape, props});
    }
    const auto markers = markerShape(object.diagnostics.markers);
    if (markers) {
      OccPartProps props;
      props.color = Quantity_Color(1.0, 0.85, 0.15, Quantity_TOC_RGB);
      props.topmost = true;
      props.markerSize = 3.0;
      props.wireframe = true;
      shapes.append({*markers, props});
    }
    if (!object.diagnostics.normalLines.isEmpty()) {
      const auto normals = makeShape({object.diagnostics.normalLines, {}});
      if (!normals) return {{}, QStringLiteral("Cannot construct diagnostic normals.")};
      OccPartProps props;
      props.color = Quantity_Color(0.2, 0.9, 0.9, Quantity_TOC_RGB);
      props.topmost = true;
      props.wireframe = true;
      props.lineWidth = 2.0;
      shapes.append({*normals, props});
    }
    for (const auto& [shape, props] : shapes) {
      const auto id = m_scene.createShapePart(shape, props);
      if (!id) { error = QStringLiteral("Cannot create drawing part."); break; }
      auto& ids = props.selectionMode == OccSelectionMode::None ? result.diagnostics : result.surfaces;
      ids.push_back(*id); // Retain identity before any OCCT display call.
      if (props.selectionMode == OccSelectionMode::None) m_scene.partHandle(*id)->SetInfiniteState(true);
      if (!m_scene.setPartVisible(*id, object.visible)) {
        error = QStringLiteral("Cannot display drawing part.");
        break;
      }
    }
  } catch (const Standard_Failure& failure) {
    error = QString::fromUtf8(failure.what());
  }
  if (!error.isEmpty()) {
    const auto cleanup = removeParts(result.surfaces, result.diagnostics);
    if (!cleanup.isEmpty()) error += QStringLiteral("; drawing cleanup: ") + cleanup;
    return {{}, error};
  }
  return {std::move(result), {}};
}

ViewportOperationResult OccSceneAdapter::insertObject(const DrawingObject& object)
{
  auto [replacement, error] = createPresentation(object);
  if (!error.isEmpty()) return {false, error};
  const auto previous = m_parts.constFind(object.id);
  if (previous != m_parts.cend()) {
    error = removeParts(previous->surfaces, previous->diagnostics);
  }
  m_parts.insert(object.id, std::move(replacement));
  return {true, error};
}

bool OccSceneAdapter::removeObject(quint32 id)
{
  const auto it = m_parts.find(id);
  if (it == m_parts.end()) return false;
  const QString error = removeParts(it->surfaces, it->diagnostics);
  if (!error.isEmpty()) qWarning().noquote() << error;
  m_parts.erase(it);
  return true;
}

ViewportOperationResult OccSceneAdapter::setObjectVisible(quint32 id, bool visible)
{
  auto it = m_parts.find(id);
  if (it == m_parts.end()) return {false, QStringLiteral("Object drawing is not installed.")};
  try {
    for (const auto part : it->surfaces)
      if (!m_scene.setPartVisible(part, visible)) return {false, QStringLiteral("Cannot change object visibility.")};
    for (const auto part : it->diagnostics)
      if (!m_scene.setPartVisible(part, visible)) return {false, QStringLiteral("Cannot change diagnostic visibility.")};
    it->visible = visible;
    return {true, {}};
  } catch (const Standard_Failure& failure) {
    return {false, QString::fromUtf8(failure.what())};
  }
}

ViewportOperationResult OccSceneAdapter::setObjectDiagnostics(
    quint32 id, const DrawingDiagnostics& diagnostics)
{
  auto it = m_parts.find(id);
  if (it == m_parts.end()) return {false, QStringLiteral("Object drawing is not installed.")};
  DrawingObject object;
  object.visible = it->visible;
  object.diagnostics = diagnostics;
  auto [replacement, error] = createPresentation(object);
  if (!error.isEmpty()) return {false, error};
  error = removeParts(it->diagnostics);
  it->diagnostics = std::move(replacement.diagnostics);
  return {true, error};
}

ViewportOperationResult OccSceneAdapter::setDraftAxis(const std::optional<DrawingAxis>& axis)
{
  DrawingObject object;
  if (axis) {
    const V3d tip = axis->center + axis->length * axis->direction;
    const V3d side = axis->direction.unitOrthogonal();
    const QList<std::array<V3d, 2>> lines{
        {axis->center, tip},
        {tip, tip - 0.25 * axis->length * axis->direction + 0.12 * axis->length * side},
        {tip, tip - 0.25 * axis->length * axis->direction - 0.12 * axis->length * side}};
    object.parts.append({lines, {{180, 35, 125}, 3.0, true}});
  }
  auto [replacement, error] = createPresentation(object);
  if (!error.isEmpty()) return {false, error};
  error = removeParts(m_axisParts);
  m_axisParts = std::move(replacement.diagnostics);
  return {true, error};
}

void OccSceneAdapter::setSelectedObjects(const QList<quint32>& ids)
{
  std::vector<OccScene::PartId> parts;
  for (const auto id : ids) {
    const auto it = m_parts.constFind(id);
    if (it != m_parts.cend() && it->visible)
      for (const auto part : it->surfaces)
        if (std::find(parts.cbegin(), parts.cend(), part) == parts.cend()) parts.push_back(part);
  }
  m_scene.selectParts(parts);
}

quint32 OccSceneAdapter::objectIdFor(const Handle(AIS_InteractiveObject)& picked) const
{
  if (picked.IsNull()) return {};
  for (auto it = m_parts.cbegin(); it != m_parts.cend(); ++it)
    for (const auto id : it->surfaces)
      if (m_scene.partHandle(id) == picked) return it.key();
  return {};
}

} // namespace RoboCrap3D
