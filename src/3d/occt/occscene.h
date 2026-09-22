#pragma once

#include "3d/math/mathtypes.h"
#include "3d/occt/occpartprops.h"

#include "occpart.h"
#include "occviewcube.h"
#include "occworldaxes.h"

#include <Standard_Handle.hxx>

#include <QString>

#include <cstddef>
#include <optional>
#include <vector>

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

  [[nodiscard]] std::optional<PartId> addShapePartWithId(const TopoDS_Shape& shape, const OccPartProps& props = {});
  [[nodiscard]] bool setPartTransform(PartId id, const M4d& transform);
  [[nodiscard]] bool setPartVisible(PartId id, bool visible);
  [[nodiscard]] bool removePart(PartId id);
  [[nodiscard]] Handle(AIS_Shape) partHandle(PartId id) const;
  void selectParts(const std::vector<PartId>& ids);

  void updateViewer();
  void updateCameraDependentObjects();
  void displayInfrastructure();
  void clearParts();

private:
  void displayWorldAxes();
  void redisplayWorldAxes();
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
  OccWorldAxes m_worldAxes;
  OccViewCube m_viewCube;
  bool m_worldAxesDisplayed = false;
  bool m_viewCubeDisplayed = false;
};

} // namespace RoboCrap3D
