#include "occviewport.h"

#include "scene/scenemodel.h"
#include "scene/sceneobject.h"
#include "scene/planegeometry.h"
#include "scene/cylindergeometry.h"

#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRep_Builder.hxx>
#include <gp_Ax3.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <QDebug>
#include <QSet>
#include <Standard_Failure.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace RoboCrap3D {

namespace {

std::optional<TopoDS_Face> makeBoundedPlaneFace(const BoundedPlane& plane)
{
  const gp_Pnt origin(plane.origin[0], plane.origin[1], plane.origin[2]);
  const gp_Dir normal(plane.coefficients[0], plane.coefficients[1], plane.coefficients[2]);
  const gp_Dir axisU(plane.axisU[0], plane.axisU[1], plane.axisU[2]);
  const gp_Pln surface(gp_Ax3(origin, normal, axisU));
  BRepBuilderAPI_MakeFace builder(surface, -plane.width / 2.0, plane.width / 2.0,
                                 -plane.height / 2.0, plane.height / 2.0);
  if (!builder.IsDone()) return std::nullopt;
  const TopoDS_Face& face = builder.Face();
  if (face.IsNull()) return std::nullopt;
  return face;
}

std::optional<TopoDS_Face> makeBoundedCylinderFace(const BoundedCylinder& cylinder)
{
  const auto& center = cylinder.origin;
  const auto& direction = cylinder.axis;
  if (!std::isfinite(center[0]) || !std::isfinite(center[1]) || !std::isfinite(center[2])
      || !std::isfinite(direction[0]) || !std::isfinite(direction[1])
      || !std::isfinite(direction[2]) || !std::isfinite(cylinder.radius)
      || !std::isfinite(cylinder.length) || cylinder.radius <= 0.0
      || cylinder.length <= 0.0) return std::nullopt;

  const double norm = std::hypot(direction[0], direction[1], direction[2]);
  if (std::abs(norm - 1.0) > 1e-6) return std::nullopt;

  // Use a stable radial reference; gp_Ax3 derives the other tangent direction.
  const bool useX = std::abs(direction[0]) < 0.9;
  const double projection = useX ? direction[0] : direction[1];
  const gp_Dir radial(useX ? 1.0 - projection * direction[0] : -projection * direction[0],
                      useX ? -projection * direction[1] : 1.0 - projection * direction[1],
                      -projection * direction[2]);
  const gp_Ax3 frame(gp_Pnt(center[0], center[1], center[2]),
                     gp_Dir(direction[0], direction[1], direction[2]), radial);
  const gp_Cylinder surface(frame, cylinder.radius);
  const double fullTurn = 2.0 * std::acos(-1.0);
  BRepBuilderAPI_MakeFace builder(surface, 0.0, fullTurn,
                                 -cylinder.length / 2.0, cylinder.length / 2.0);
  if (!builder.IsDone()) return std::nullopt;
  const TopoDS_Face& face = builder.Face();
  if (face.IsNull()) return std::nullopt;
  return face;
}

std::optional<TopoDS_Compound> makeDiagnosticShape(const SceneObject* object, bool normals)
{
  using Point = BoundedPlane::Point;
  const std::vector<Point>* samples = nullptr;
  const BoundedPlane* plane = nullptr;
  const BoundedCylinder* cylinder = nullptr;
  if (object->kind() == SceneObject::Plane) {
    const auto* geometry = qobject_cast<const PlaneGeometry*>(object->geometry());
    if (!geometry || !geometry->hasBounds()) return std::nullopt;
    plane = geometry->boundedPlane().get();
    samples = &plane->points;
  } else if (object->kind() == SceneObject::Cylinder) {
    const auto* geometry = qobject_cast<const CylinderGeometry*>(object->geometry());
    if (!geometry) return std::nullopt;
    cylinder = geometry->boundedCylinder().get();
    samples = &cylinder->points;
  } else {
    return std::nullopt;
  }
  if (samples->empty()) return std::nullopt;

  constexpr std::size_t kMaxMarkers = 256;
  constexpr std::size_t kMaxNormals = 128;
  const std::size_t limit = normals ? kMaxNormals : kMaxMarkers;
  const std::size_t stride = 1 + (samples->size() - 1) / limit;
  const double length = plane
      ? std::clamp(0.05 * std::min(plane->width, plane->height), 1.0, 20.0)
      : std::clamp(0.25 * cylinder->radius, 1.0, 20.0);
  BRep_Builder builder;
  TopoDS_Compound compound;
  builder.MakeCompound(compound);
  std::size_t count = 0;
  for (std::size_t i = 0; i < samples->size(); i += stride) {
    const Point& point = (*samples)[i];
    const gp_Pnt start(point[0], point[1], point[2]);
    if (!normals) {
      BRepBuilderAPI_MakeVertex vertex(start);
      builder.Add(compound, vertex.Vertex());
      ++count;
      continue;
    }

    Point direction{};
    if (plane) {
      direction = {plane->coefficients[0], plane->coefficients[1], plane->coefficients[2]};
    } else {
      double axial = 0.0;
      for (int j = 0; j < 3; ++j)
        axial += (point[j] - cylinder->origin[j]) * cylinder->axis[j];
      double norm2 = 0.0;
      for (int j = 0; j < 3; ++j) {
        direction[j] = point[j] - cylinder->origin[j] - axial * cylinder->axis[j];
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

OccViewport::OccViewport(Aspect_Handle handle, const RobotPreviewState& state,
                         const CadLoadResult& shapes, SceneModel* applicationScene)
  : m_viewer(handle),
    m_scene(m_viewer.context(), m_viewer.view()),
    m_robot(m_scene),
    m_input(m_viewer.context(), m_viewer.view())
{
  m_timer.setSingleShot(true);
  m_timer.setInterval(0);
  QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this]() { flushRender(); });
  if (!m_viewer.isValid()) return;
  m_scene.displayInfrastructure();
  if (!m_robot.load(state.model(), shapes) || !m_robot.applyTransforms(state.pose().transforms)) return;
  synchronizeApplicationScene(applicationScene);
  m_viewer.fitAll();
  m_scene.updateCameraDependentObjects();
  m_ready = true;
}

void OccViewport::synchronizeApplicationScene(SceneModel* applicationScene)
{
  QSet<QString> liveIds;
  if (applicationScene) {
    for (SceneObject* object : applicationScene->objectList()) liveIds.insert(object->objectId());
  }

  bool changed = false;
  for (auto it = m_applicationParts.begin(); it != m_applicationParts.end();) {
    if (liveIds.contains(it.key())) {
      ++it;
      continue;
    }
    changed = m_scene.removePart(it.value()) || changed;
    const auto overlay = m_applicationOverlays.take(it.key());
    if (overlay.points) changed = m_scene.removePart(*overlay.points) || changed;
    if (overlay.normals) changed = m_scene.removePart(*overlay.normals) || changed;
    it = m_applicationParts.erase(it);
  }

  if (applicationScene) {
    for (SceneObject* object : applicationScene->objectList()) {
      auto it = m_applicationParts.constFind(object->objectId());
      bool created = false;
      if (it == m_applicationParts.cend()) {
        try {
          std::optional<TopoDS_Face> face;
          if (object->kind() == SceneObject::Plane) {
            const auto* geometry = qobject_cast<const PlaneGeometry*>(object->geometry());
            if (!geometry || !geometry->hasBounds()) continue;
            face = makeBoundedPlaneFace(*geometry->boundedPlane());
          } else if (object->kind() == SceneObject::Cylinder) {
            const auto* geometry = qobject_cast<const CylinderGeometry*>(object->geometry());
            if (!geometry) continue;
            face = makeBoundedCylinderFace(*geometry->boundedCylinder());
          } else {
            continue;
          }
          if (!face) {
            qWarning() << "Cannot create bounded surface face for scene object:" << object->objectId();
            continue;
          }
          OccPartProps props;
          props.color = kRoughSurfaceColor;
          props.selectionMode = OccSelectionMode::All;
          const auto partId = m_scene.addShapePartWithId(*face, props);
          if (!partId) {
            qWarning() << "Cannot display bounded surface for scene object:" << object->objectId();
            continue;
          }
          m_applicationParts.insert(object->objectId(), *partId);
          it = m_applicationParts.constFind(object->objectId());
          created = true;
          changed = true;
        } catch (const Standard_Failure& failure) {
          qWarning() << "OCCT bounded surface construction failed for scene object:"
                     << object->objectId() << failure.what();
          continue;
        }
      }
      if (!created || !object->visible()) {
        changed = m_scene.setPartVisible(it.value(), object->visible()) || changed;
      }
      auto& overlays = m_applicationOverlays[object->objectId()];
      changed = synchronizeOverlay(object, overlays.points,
                                   m_showPoints && object->visible(), false) || changed;
      changed = synchronizeOverlay(object, overlays.normals,
                                   m_showNormals && object->visible(), true) || changed;
    }
  }

  if (changed) requestRender();
}

bool OccViewport::synchronizeOverlay(const SceneObject* object,
                                     std::optional<OccScene::PartId>& partId,
                                     bool enabled, bool normals)
{
  if (!partId && !enabled) return false;
  if (!partId) {
    try {
      const auto shape = makeDiagnosticShape(object, normals);
      if (!shape) return false;
      OccPartProps props;
      props.color = normals ? Quantity_Color(0.2, 0.9, 0.9, Quantity_TOC_RGB)
                            : Quantity_Color(1.0, 0.85, 0.15, Quantity_TOC_RGB);
      props.selectionMode = OccSelectionMode::None;
      props.wireframe = true;
      props.topmost = true;
      props.markerSize = normals ? 0.0 : 3.0;
      props.lineWidth = normals ? 2.0 : 0.0;
      partId = m_scene.addShapePartWithId(*shape, props);
      return partId.has_value();
    } catch (const Standard_Failure& failure) {
      qWarning() << "OCCT diagnostic overlay failed for scene object:"
                 << object->objectId() << failure.what();
      return false;
    }
  }
  return m_scene.setPartVisible(*partId, enabled);
}

void OccViewport::setDiagnosticOverlays(bool showPoints, bool showNormals,
                                        SceneModel* applicationScene)
{
  if (m_showPoints == showPoints && m_showNormals == showNormals) return;
  m_showPoints = showPoints;
  m_showNormals = showNormals;
  synchronizeApplicationScene(applicationScene);
}

void OccViewport::setSelectedObjects(const QStringList& ids)
{
  std::vector<OccScene::PartId> parts;
  parts.reserve(static_cast<std::size_t>(ids.size()));
  for (const QString& id : ids) {
    const auto it = m_applicationParts.constFind(id);
    if (it != m_applicationParts.cend()
        && std::find(parts.cbegin(), parts.cend(), it.value()) == parts.cend())
      parts.push_back(it.value());
  }
  m_scene.selectParts(parts);
  requestRender();
}

OccViewport::~OccViewport()
{
  m_timer.stop();
}

bool OccViewport::isValid() const
{
  return m_ready && m_viewer.isValid();
}

bool OccViewport::applyPose(const RobotPose& pose)
{
  if (!isValid() || !m_robot.applyTransforms(pose.transforms)) return false;
  requestRender();
  return true;
}

void OccViewport::setExposed(bool exposed)
{
  m_exposed = exposed;
  if (exposed) {
    resize();
  } else {
    m_timer.stop();
    cancelGesture();
  }
}

void OccViewport::resize()
{
  if (!m_exposed || !isValid()) return;
  m_viewer.resize();
  m_scene.updateCameraDependentObjects();
  requestRender();
}

std::optional<QString> OccViewport::mousePress(const QPoint& pos, Qt::MouseButton button)
{
  if (!m_exposed) return std::nullopt;
  const OccInputResult input = m_input.mousePress(pos, button);
  applyInput(input);
  if (button != Qt::LeftButton || !input.selectionChanged) return std::nullopt;
  if (m_viewer.context()->HasDetected()) {
    const auto detected = m_viewer.context()->DetectedInteractive();
    for (auto it = m_applicationParts.cbegin(); it != m_applicationParts.cend(); ++it) {
      if (m_scene.partHandle(it.value()) == detected) return it.key();
    }
  }
  return QString{}; // Background or non-application geometry clears UI selection.
}

void OccViewport::mouseMove(const QPoint& pos)
{
  if (m_exposed) applyInput(m_input.mouseMove(pos));
}

void OccViewport::mouseRelease(Qt::MouseButton button)
{
  applyInput(m_input.mouseRelease(button));
}

void OccViewport::wheel(const QPoint& pos, int delta)
{
  if (m_exposed) applyInput(m_input.wheel(pos, delta));
}

void OccViewport::cancelGesture()
{
  m_input.cancelGesture();
}

void OccViewport::applyInput(const OccInputResult& input)
{
  if (!isValid() || !input.accepted) return;
  if (input.cameraScaleChanged) m_scene.updateCameraDependentObjects();
  if (input.needsRedraw || input.needsViewerUpdate) requestRender();
}

void OccViewport::requestRender()
{
  if (m_exposed && isValid() && !m_timer.isActive()) m_timer.start();
}

void OccViewport::flushRender()
{
  if (!m_exposed || !isValid()) return;
  try {
    // UpdateCurrentViewer redraws the viewer; do not immediately redraw it twice.
    m_viewer.updateCurrentViewer();
  } catch (const Standard_Failure& failure) {
    qWarning() << "OCCT redraw failed:" << failure.what();
  }
}

} // namespace RoboCrap3D
