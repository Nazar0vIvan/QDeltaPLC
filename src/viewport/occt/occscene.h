#pragma once

#include "occt/occpartprops.h"

#include "occpart.h"
#include "occviewcube.h"
#include "occworldaxes.h"

#include <Standard_Handle.hxx>

#include <cstddef>
#include <optional>
#include <vector>
#include <map>
#include <utility>

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

  // Creation returns ownership identity before any display call can fail.
  [[nodiscard]] std::optional<PartId> createShapePart(const TopoDS_Shape& shape, const OccPartProps& props);
  [[nodiscard]] std::optional<PartId> addShapePartWithId(const TopoDS_Shape& shape, const OccPartProps& props = {});
  [[nodiscard]] bool setPartTransform(PartId id, const gp_Trsf& transform);
  [[nodiscard]] bool setPartVisible(PartId id, bool visible);
  [[nodiscard]] bool removePart(PartId id);
  [[nodiscard]] Handle(AIS_Shape) partHandle(PartId id) const;
  void selectParts(const std::vector<PartId>& ids);

  void updateCameraDependentObjects();
  // BASE-relative frame; empty hides the non-selectable TCP axes.
  void setSpindleTcpFrame(const std::optional<gp_Trsf>& frame);
  // BASE-relative frame; empty hides the non-selectable ER axes.
  void setSpindleColletFrame(const std::optional<gp_Trsf>& frame);
  void displayInfrastructure();

private:
  void displayWorldAxes();
  void redisplayWorldAxes();
  void setFrameAxes(OccWorldAxes& axes, const std::optional<gp_Trsf>& frame);
  void resizeFrameAxes(OccWorldAxes& axes);
  void displayViewCube();
  bool displayPart(OccPart& part);
  void activateAllSelectionModes(const OccPart& part);
  void unselectPart(PartId id);
  [[nodiscard]] double currentWorldAxisLength() const;

private:
  Handle(AIS_InteractiveContext) m_context;
  Handle(V3d_View) m_view;
  // Only live parts are retained; IDs, including zero, are never reused or reset after removal.
  std::map<PartId, OccPart> m_parts;
  std::optional<PartId> m_nextPartId = 0;
  // Selected IDs with outline/completion state; curves have a null outline.
  std::map<PartId, std::pair<Handle(AIS_Shape), bool>> m_selectedParts;
  OccWorldAxes m_worldAxes;
  OccWorldAxes m_spindleTcpAxes;
  OccWorldAxes m_spindleColletAxes;
  OccViewCube m_viewCube;
  bool m_worldAxesDisplayed = false;
  bool m_viewCubeDisplayed = false;
};

} // namespace RoboCrap3D
