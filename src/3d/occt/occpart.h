#pragma once

#include <AIS_Shape.hxx>
#include <AIS_Trihedron.hxx>

#include <Quantity_Color.hxx>

#include <TopoDS_Shape.hxx>

#include <gp_Trsf.hxx>

#include "3d/occt/occpartprops.h"

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
  [[nodiscard]] const Handle(AIS_Trihedron)& trihedron() const;

  [[nodiscard]] const Quantity_Color& color() const;
  void setColor(const Quantity_Color& color);

  [[nodiscard]] const gp_Trsf& transform() const;
  void setTransform(const gp_Trsf& transform);

  [[nodiscard]] OccSelectionMode selectionMode() const;
  [[nodiscard]] bool topmost() const { return m_topmost; }
  void setSelectionMode(OccSelectionMode selectionMode);

  [[nodiscard]] bool hasTrihedron() const;
  void enableTrihedron(double size = 20.0);
  void disableTrihedron();

private:
  void createPresentation(const TopoDS_Shape& shape);
  void configureBasePresentation();
  void configureFaceBoundary();

  void applyColor();
  void applyTransform();

  void rebuildTrihedron();
  void configureTrihedron();

private:
  Handle(AIS_Shape) m_handle;
  Handle(AIS_Trihedron) m_trihedron;

  Quantity_Color m_color;
  gp_Trsf m_transform;

  OccSelectionMode m_selectionMode;
  bool m_wireframe = false;
  bool m_topmost = false;
  double m_markerSize = 0.0;
  double m_lineWidth = 0.0;

  bool m_trihedronEnabled = false;
  double m_trihedronSize = 20.0;
};

} // namespace RoboCrap3D
