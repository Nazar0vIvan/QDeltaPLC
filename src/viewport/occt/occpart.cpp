#include "occpart.h"

#include <AIS_DisplayMode.hxx>
#include <Aspect_TypeOfLine.hxx>
#include <Aspect_TypeOfMarker.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Quantity_NameOfColor.hxx>

namespace RoboCrap3D {

OccPart::OccPart(const TopoDS_Shape& shape, const OccPartProps& props)
    : m_selectionMode(props.selectionMode), m_topmost(props.topmost), m_faceBoundary(props.faceBoundary)
{
  if (shape.IsNull()) return;
  m_handle = new AIS_Shape(shape);
  m_handle->SetDisplayMode(props.wireframe ? AIS_WireFrame : AIS_Shaded);
  m_handle->SetMaterial(Graphic3d_NOM_SATIN);
  if (props.markerSize > 0.0 || props.lineWidth > 0.0) {
    Handle(Prs3d_Drawer) drawer = m_handle->Attributes();
    if (drawer.IsNull()) {
      drawer = new Prs3d_Drawer();
      m_handle->SetAttributes(drawer);
    }
    if (props.markerSize > 0.0)
      drawer->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_BALL, props.color, props.markerSize));
    if (props.lineWidth > 0.0)
      drawer->SetLineAspect(new Prs3d_LineAspect(props.color, Aspect_TOL_SOLID, props.lineWidth));
  }
  m_handle->SetColor(props.color);
  if (props.transparency > 0.0) m_handle->SetTransparency(props.transparency);
  Handle(Prs3d_Drawer) drawer = m_handle->Attributes();
  if (drawer.IsNull()) {
    drawer = new Prs3d_Drawer();
    m_handle->SetAttributes(drawer);
  }
  drawer->SetFaceBoundaryDraw(props.faceBoundary);
  drawer->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.5));
  m_handle->SetLocalTransformation(props.transform);
}

bool OccPart::isValid() const
{
  return !m_handle.IsNull();
}

const Handle(AIS_Shape)& OccPart::handle() const
{
  return m_handle;
}

void OccPart::setTransform(const gp_Trsf& transform)
{
  if (!m_handle.IsNull()) m_handle->SetLocalTransformation(transform);
}

OccSelectionMode OccPart::selectionMode() const
{
  return m_selectionMode;
}

} // namespace RoboCrap3D
