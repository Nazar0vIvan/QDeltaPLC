#pragma once

#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>

#include "occt/occpartprops.h"

namespace RoboCrap3D {

class OccPart final
{
public:
  explicit OccPart(const TopoDS_Shape& shape, const OccPartProps& props);
  ~OccPart() = default;

  OccPart(const OccPart&) = delete;
  OccPart& operator=(const OccPart&) = delete;
  OccPart(OccPart&&) noexcept = default;
  OccPart& operator=(OccPart&&) noexcept = default;

  [[nodiscard]] bool isValid() const;
  [[nodiscard]] const Handle(AIS_Shape)& handle() const;
  void setTransform(const gp_Trsf& transform);
  [[nodiscard]] OccSelectionMode selectionMode() const;
  [[nodiscard]] bool topmost() const { return m_topmost; }

private:
  Handle(AIS_Shape) m_handle;
  OccSelectionMode m_selectionMode;
  bool m_topmost = false;
};

} // namespace RoboCrap3D
