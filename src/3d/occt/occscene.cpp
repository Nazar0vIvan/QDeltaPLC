#include "occscene.h"

#include <QDebug>

#include <AIS_InteractiveContext.hxx>
#include <AIS_SelectionModesConcurrency.hxx>
#include <AIS_Shape.hxx>

#include <Graphic3d_ZLayerId.hxx>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <V3d_View.hxx>

namespace RoboCrap3D {

OccScene::OccScene(
    const Handle(AIS_InteractiveContext)& context,
    const Handle(V3d_View)& view)
    : m_context(context)
      , m_view(view)
      , m_worldAxes(currentWorldAxisLength())
      , m_viewCube()
{}

bool OccScene::isValid() const
{
  return !m_context.IsNull() && !m_view.IsNull();
}

void OccScene::updateCameraDependentObjects()
{
  if (!isValid()) return;

  if (!m_worldAxesDisplayed) {
    displayWorldAxes();
    return;
  }

  m_worldAxes.setLength(currentWorldAxisLength());
  redisplayWorldAxes();
}

void OccScene::displayInfrastructure()
{
  displayViewCube();
  displayWorldAxes();
}

void OccScene::displayWorldAxes()
{
  if (!isValid()) return;

  if (m_worldAxesDisplayed) return;

  if (!m_worldAxes.isValid()) {
    qWarning() << "Cannot display world axes: OccWorldAxes is invalid";
    return;
  }

  m_worldAxes.setLength(currentWorldAxisLength());

  m_context->Display(m_worldAxes.xAxis(), false);
  m_context->Display(m_worldAxes.yAxis(), false);
  m_context->Display(m_worldAxes.zAxis(), false);

  m_context->Deactivate(m_worldAxes.xAxis());
  m_context->Deactivate(m_worldAxes.yAxis());
  m_context->Deactivate(m_worldAxes.zAxis());

  m_worldAxesDisplayed = true;
}

void OccScene::redisplayWorldAxes()
{
  if (!isValid()) return;

  if (!m_worldAxesDisplayed) return;

  if (!m_worldAxes.isValid()) return;

  m_context->Redisplay(m_worldAxes.xAxis(), false);
  m_context->Redisplay(m_worldAxes.yAxis(), false);
  m_context->Redisplay(m_worldAxes.zAxis(), false);
}

void OccScene::displayViewCube()
{
  if (!isValid()) return;

  if (m_viewCubeDisplayed) return;

  if (!m_viewCube.isValid()) {
    qWarning() << "Cannot display ViewCube: OccViewCube is invalid";
    return;
  }

  m_context->Display(m_viewCube.handle(), false);
  m_context->Activate(m_viewCube.handle(), 0, false);

  m_viewCubeDisplayed = true;
}

bool OccScene::displayPart(OccPart& part)
{
  if (!isValid()) return false;

  if (!part.isValid()) return false;

  const int selectionMode = part.selectionMode() == OccSelectionMode::None ? -1 : 0;

  m_context->Display(part.handle(), part.handle()->DisplayMode(), selectionMode, false);

  if (part.topmost()) m_context->SetZLayer(part.handle(), Graphic3d_ZLayerId_Topmost);

  if (part.selectionMode() == OccSelectionMode::All) {
    activateAllSelectionModes(part);
  }

  if (part.hasTrihedron()) {
    m_context->Display(part.trihedron(), false);
    m_context->SetZLayer(part.trihedron(), Graphic3d_ZLayerId_Topmost);
    m_context->Deactivate(part.trihedron());
  }

  return true;
}

void OccScene::activateAllSelectionModes(const OccPart& part)
{
  if (!isValid()) return;

  if (!part.isValid()) return;

  m_context->SetSelectionModeActive(
    part.handle(),
    AIS_Shape::SelectionMode(TopAbs_FACE),
    true,
    AIS_SelectionModesConcurrency_Multiple
  );

  m_context->SetSelectionModeActive(
    part.handle(),
    AIS_Shape::SelectionMode(TopAbs_EDGE),
    true,
    AIS_SelectionModesConcurrency_Multiple
  );

  m_context->SetSelectionModeActive(
    part.handle(),
    AIS_Shape::SelectionMode(TopAbs_VERTEX),
    true,
    AIS_SelectionModesConcurrency_Multiple
  );
}

double OccScene::currentWorldAxisLength() const
{
  if (m_view.IsNull() || m_view->Camera().IsNull()) {
    return 10.0;
  }

  const double cameraScale = m_view->Camera()->Scale();

  if (cameraScale <= 0.0) {
    return 10.0;
  }

  return cameraScale * 0.05;
}

std::optional<OccScene::PartId> OccScene::addShapePartWithId(const TopoDS_Shape& shape, const OccPartProps& props)
{
  if (!isValid()) {
    qWarning() << "Cannot add OCCT shape part: context or view is null";
    return std::nullopt;
  }

  if (shape.IsNull()) {
    qWarning() << "Cannot add OCCT shape part: TopoDS_Shape is null";
    return std::nullopt;
  }

  OccPart part(shape, props);

  if (!part.isValid()) {
    qWarning() << "Cannot add OCCT shape part: AIS presentation was not created";
    return std::nullopt;
  }

  const PartId id = m_parts.size();
  m_parts.emplace_back(std::move(part));
  try {
    if (!displayPart(*m_parts.back())) {
      (void)removePart(id);
      return std::nullopt;
    }
  } catch (...) {
    (void)removePart(id);
    throw;
  }

  return id;
}

bool OccScene::setPartTransform(const PartId id, const M4d& transform)
{
  if (id >= m_parts.size() || !m_parts[id]) {
    qWarning() << "Invalid OCCT scene part id:" << id;
    return false;
  }

  m_parts[id]->setTransform(transform);
  return true;
}

bool OccScene::setPartVisible(const PartId id, const bool visible)
{
  if (!isValid() || id >= m_parts.size() || !m_parts[id]) {
    qWarning() << "Invalid OCCT scene part id:" << id;
    return false;
  }

  OccPart& part = *m_parts[id];
  if (visible) {
    return displayPart(part);
  }

  m_context->Erase(part.handle(), false);
  if (part.hasTrihedron()) m_context->Erase(part.trihedron(), false);
  return true;
}

bool OccScene::removePart(const PartId id)
{
  if (!isValid() || id >= m_parts.size() || !m_parts[id]) {
    qWarning() << "Invalid OCCT scene part id:" << id;
    return false;
  }

  const OccPart& part = *m_parts[id];
  m_context->Remove(part.handle(), false);
  if (part.hasTrihedron()) m_context->Remove(part.trihedron(), false);
  m_parts[id].reset();
  return true;
}

Handle(AIS_Shape) OccScene::partHandle(const PartId id) const
{
  return id < m_parts.size() && m_parts[id] ? m_parts[id]->handle() : Handle(AIS_Shape){};
}

void OccScene::selectParts(const std::vector<PartId>& ids)
{
  if (!isValid()) return;
  m_context->ClearSelected(false);
  for (const PartId id : ids) {
    const Handle(AIS_Shape) handle = partHandle(id);
    if (!handle.IsNull() && m_context->IsDisplayed(handle))
      m_context->AddOrRemoveSelected(handle, false);
  }
}

void OccScene::updateViewer()
{
  if (!m_context.IsNull()) {
    m_context->UpdateCurrentViewer();
  }
}

void OccScene::clearParts()
{
  if (!isValid()) return;
  for (auto& part : m_parts) {
    if (!part) continue;
    m_context->Remove(part->handle(), false);
    if (part->hasTrihedron()) m_context->Remove(part->trihedron(), false);
    part.reset();
  }
}

} // namespace RoboCrap3D
