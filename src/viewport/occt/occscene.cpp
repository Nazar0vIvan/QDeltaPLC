#include "occscene.h"

#include <QDebug>
#include <algorithm>
#include <limits>

#include <AIS_InteractiveContext.hxx>
#include <AIS_SelectionModesConcurrency.hxx>
#include <AIS_Shape.hxx>

#include <Graphic3d_ZLayerId.hxx>
#include <Prs3d_Drawer.hxx>
#include <PrsMgr_Presentation.hxx>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Builder.hxx>

#include <V3d_View.hxx>

namespace RoboCrap3D {

OccScene::OccScene(
    const Handle(AIS_InteractiveContext)& context,
    const Handle(V3d_View)& view)
    : m_context(context)
      , m_view(view)
      , m_worldAxes(currentWorldAxisLength())
      , m_spindleTcpAxes(currentWorldAxisLength())
      , m_spindleColletAxes(currentWorldAxisLength())
      , m_viewCube()
{}

bool OccScene::isValid() const
{
  return !m_context.IsNull() && !m_view.IsNull();
}

bool OccScene::hasVisibleParts() const
{
  if (!isValid()) return false;
  for (const auto& [id, part] : m_parts) {
    if (!part.handle()->IsInfinite() && m_context->IsDisplayed(part.handle())) return true;
  }
  return false;
}

void OccScene::updateCameraDependentObjects()
{
  if (!isValid()) return;

  if (!m_worldAxesDisplayed) {
    if (hasVisibleParts()) displayWorldAxes();
  } else {
    m_worldAxes.setLength(currentWorldAxisLength());
    redisplayWorldAxes();
  }
  resizeFrameAxes(m_spindleTcpAxes);
  resizeFrameAxes(m_spindleColletAxes);
}

void OccScene::setSpindleTcpFrame(const std::optional<gp_Trsf>& frame)
{
  setFrameAxes(m_spindleTcpAxes, frame);
}

void OccScene::setSpindleColletFrame(const std::optional<gp_Trsf>& frame)
{
  setFrameAxes(m_spindleColletAxes, frame);
}

void OccScene::setFrameAxes(OccWorldAxes& axes, const std::optional<gp_Trsf>& frame)
{
  if (!isValid() || !axes.isValid()) return;
  const bool displayed = m_context->IsDisplayed(axes.xAxis());
  if (!frame) {
    if (!displayed) return;
    for (const auto& axis : {axes.xAxis(), axes.yAxis(), axes.zAxis()})
      m_context->Erase(axis, false);
    return;
  }
  for (const auto& axis : {axes.xAxis(), axes.yAxis(), axes.zAxis()})
    axis->SetLocalTransformation(*frame);
  if (displayed) return;
  axes.setLength(currentWorldAxisLength());
  for (const auto& axis : {axes.xAxis(), axes.yAxis(), axes.zAxis()}) {
    m_context->Display(axis, 0, -1, false);
    m_context->Deactivate(axis);
    m_context->Redisplay(axis, false);
  }
}

void OccScene::resizeFrameAxes(OccWorldAxes& axes)
{
  if (!axes.isValid() || !m_context->IsDisplayed(axes.xAxis())) return;
  axes.setLength(currentWorldAxisLength());
  for (const auto& axis : {axes.xAxis(), axes.yAxis(), axes.zAxis()})
    m_context->Redisplay(axis, false);
}

void OccScene::displayInfrastructure()
{
  displayViewCube();
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

  // Transparent parts keep their transparency when selected or hovered, so parts behind them
  // stay visible; opaque parts use the viewport styles of OccViewer::configureHighlightStyles.
  const Handle(AIS_Shape)& handle = part.handle();
  if (handle->IsTransparent()) {
    const Handle(Prs3d_Drawer) selected = new Prs3d_Drawer(*m_context->HighlightStyle(Prs3d_TypeOfHighlight_Selected));
    const Handle(Prs3d_Drawer) hover = new Prs3d_Drawer(*m_context->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic));
    selected->SetTransparency(static_cast<float>(handle->Transparency()));
    hover->SetTransparency(static_cast<float>(handle->Transparency()));
    handle->SetHilightAttributes(selected);
    handle->SetDynamicHilightAttributes(hover);
  }

  m_context->Display(handle, handle->DisplayMode(), selectionMode, false);
  // FitAll skips infinite structures. Marking the AIS object infinite instead would make
  // AIS_Shape compute its shaded presentation as wireframe.
  if (part.fitExcluded())
    for (const auto& presentation : handle->Presentations()) presentation->SetInfiniteState(true);

  if (part.topmost()) m_context->SetZLayer(handle, Graphic3d_ZLayerId_Topmost);

  if (part.selectionMode() == OccSelectionMode::All) {
    activateAllSelectionModes(part);
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

std::optional<OccScene::PartId> OccScene::createShapePart(const TopoDS_Shape& shape, const OccPartProps& props)
{
  if (!isValid()) {
    qWarning() << "Cannot add OCCT shape part: context or view is null";
    return std::nullopt;
  }

  if (shape.IsNull()) {
    qWarning() << "Cannot add OCCT shape part: TopoDS_Shape is null";
    return std::nullopt;
  }

  if (!m_nextPartId) {
    qWarning() << "Cannot add OCCT shape part: part IDs are exhausted";
    return std::nullopt;
  }

  OccPart part(shape, props);

  if (!part.isValid()) {
    qWarning() << "Cannot add OCCT shape part: AIS presentation was not created";
    return std::nullopt;
  }

  const PartId id = *m_nextPartId;
  m_parts.emplace(id, std::move(part));
  if (id == std::numeric_limits<PartId>::max()) m_nextPartId.reset();
  else ++*m_nextPartId;
  return id;
}

bool OccScene::setPartTransform(const PartId id, const gp_Trsf& transform)
{
  const auto found = m_parts.find(id);
  if (found == m_parts.end()) {
    qWarning() << "Invalid OCCT scene part id:" << id;
    return false;
  }

  found->second.setTransform(transform);
  if (const auto selected = m_selectedParts.find(id);
      selected != m_selectedParts.end() && !selected->second.first.IsNull())
    selected->second.first->SetLocalTransformation(transform);
  return true;
}

bool OccScene::setPartVisible(const PartId id, const bool visible)
{
  const auto found = m_parts.find(id);
  if (!isValid() || found == m_parts.end()) {
    qWarning() << "Invalid OCCT scene part id:" << id;
    return false;
  }

  OccPart& part = found->second;
  if (!visible) unselectPart(id);
  if (m_context->IsDisplayed(part.handle()) == visible) return true;
  if (visible) {
    return displayPart(part);
  }

  m_context->Erase(part.handle(), false);
  return true;
}

bool OccScene::removePart(const PartId id)
{
  const auto found = m_parts.find(id);
  if (!isValid() || found == m_parts.end()) {
    qWarning() << "Invalid OCCT scene part id:" << id;
    return false;
  }

  const OccPart& part = found->second;
  unselectPart(id);
  m_context->Remove(part.handle(), false);
  m_parts.erase(found);
  return true;
}

Handle(AIS_Shape) OccScene::partHandle(const PartId id) const
{
  const auto found = m_parts.find(id);
  return found != m_parts.end() ? found->second.handle() : Handle(AIS_Shape){};
}

void OccScene::unselectPart(const PartId id)
{
  const auto selected = m_selectedParts.find(id);
  if (selected == m_selectedParts.end()) return;
  auto& [outline, ready] = selected->second;
  ready = false; // A failed removal must not leave a completed display record.
  if (!outline.IsNull()) m_context->Remove(outline, false);
  const auto handle = partHandle(id);
  if (!handle.IsNull() && m_context->IsSelected(handle))
    m_context->AddOrRemoveSelected(handle, false);
  m_selectedParts.erase(selected);
}

void OccScene::selectParts(const std::vector<PartId>& ids)
{
  if (!isValid()) return;
  for (auto selected = m_selectedParts.begin(); selected != m_selectedParts.end();) {
    const PartId id = (selected++)->first;
    const auto handle = partHandle(id);
    if (std::find(ids.cbegin(), ids.cend(), id) == ids.cend()
        || handle.IsNull() || !m_context->IsDisplayed(handle)) unselectPart(id);
  }
  for (const PartId id : ids) {
    const Handle(AIS_Shape) handle = partHandle(id);
    if (handle.IsNull() || !m_context->IsDisplayed(handle)) continue;
    if (!m_context->IsSelected(handle)) m_context->AddOrRemoveSelected(handle, false);
    auto& [outline, ready] = m_selectedParts.try_emplace(id, Handle(AIS_Shape){}, false).first->second;
    if (ready) continue;
    // Highlight color overrides face-boundary aspects. Keep the boundary in
    // its own unhighlighted presentation; standalone curves remain amber.
    // The non-null handle above was found in m_parts.
    if (outline.IsNull() && m_parts.find(id)->second.faceBoundary()
        && TopExp_Explorer(handle->Shape(), TopAbs_FACE).More()) {
      BRep_Builder builder;
      TopoDS_Compound boundaries;
      builder.MakeCompound(boundaries);
      for (TopExp_Explorer edge(handle->Shape(), TopAbs_EDGE); edge.More(); edge.Next())
        builder.Add(boundaries, edge.Current());
      outline = new AIS_Shape(boundaries); // Own it before any display/style operation can fail.
    }
    if (!outline.IsNull()) {
      outline->SetColor(Quantity_Color(48.0 / 255.0, 48.0 / 255.0, 48.0 / 255.0, Quantity_TOC_sRGB));
      outline->SetWidth(2.0);
      outline->SetLocalTransformation(handle->LocalTransformation());
      outline->SetInfiniteState(true); // Decoration must not affect camera fitting.
      m_context->Display(outline, 0, -1, false);
      // Share scene depth so rear boundaries are hidden by opaque faces.
      m_context->SetZLayer(outline, Graphic3d_ZLayerId_Top);
    }
    ready = true;
  }
}

} // namespace RoboCrap3D
