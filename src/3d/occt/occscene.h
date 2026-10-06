#pragma once

#include "3d/occt/occpartprops.h"

#include "occpart.h"
#include "occviewcube.h"
#include "occworldaxes.h"

#include <Standard_Handle.hxx>

#include <cstddef>
#include <optional>
#include <vector>
#include <map>

class AIS_InteractiveContext;
class V3d_View;
class TopoDS_Shape;

namespace RoboCrap3D {

class OccScene final
{
public:
  using PartId = std::size_t;

  OccScene(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view);
  ~OccScene() = default;

  OccScene(const OccScene&) = delete;
  OccScene& operator=(const OccScene&) = delete;

  OccScene(OccScene&&) noexcept = delete;
  OccScene& operator=(OccScene&&) noexcept = delete;

  [[nodiscard]] bool isValid() const;
  [[nodiscard]] bool hasVisibleParts() const;

  [[nodiscard]] std::optional<PartId> addShapePartWithId(const TopoDS_Shape& shape, const OccPartProps& props = {});
  [[nodiscard]] bool setPartTransform(PartId id, const gp_Trsf& transform);
  [[nodiscard]] bool setPartVisible(PartId id, bool visible);
  [[nodiscard]] bool removePart(PartId id);
  [[nodiscard]] Handle(AIS_Shape) partHandle(PartId id) const;
  void selectParts(const std::vector<PartId>& ids);

  void updateViewer();
  void updateCameraDependentObjects();
  // BASE-relative frame; empty hides the non-selectable TCP axes.
  void setSpindleTcpFrame(const std::optional<gp_Trsf>& frame);
  // BASE-relative frame; empty hides the non-selectable ER axes.
  void setSpindleColletFrame(const std::optional<gp_Trsf>& frame);
  void displayInfrastructure();
  void clearParts();

private:
  void displayWorldAxes();
  void redisplayWorldAxes();
  void setFrameAxes(OccWorldAxes& axes, const std::optional<gp_Trsf>& frame);
  void resizeFrameAxes(OccWorldAxes& axes);
  void displayViewCube();
  bool displayPart(OccPart& part);
  void activateAllSelectionModes(const OccPart& part);
  [[nodiscard]] double currentWorldAxisLength() const;

private:
  Handle(AIS_InteractiveContext) m_context;
  Handle(V3d_View) m_view;
  // Slots are never reused during this scene's lifetime. A removed part leaves
  // a tombstone so a stale PartId cannot address a later presentation.
  std::vector<std::optional<OccPart>> m_parts;
  std::map<PartId, Handle(AIS_Shape)> m_selectionOutlines;
  OccWorldAxes m_worldAxes;
  OccWorldAxes m_spindleTcpAxes;
  OccWorldAxes m_spindleColletAxes;
  OccViewCube m_viewCube;
  bool m_worldAxesDisplayed = false;
  bool m_viewCubeDisplayed = false;
};

} // namespace RoboCrap3D
