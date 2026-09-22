#include "occpart.h"

#include <AIS_Shape.hxx>
#include <AIS_Trihedron.hxx>

#include <Aspect_TypeOfLine.hxx>
#include <Aspect_TypeOfMarker.hxx>

#include <Geom_Axis2Placement.hxx>

#include <Graphic3d_NameOfMaterial.hxx>

#include <Prs3d_DatumAspect.hxx>
#include <Prs3d_DatumParts.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_PointAspect.hxx>

#include <Quantity_NameOfColor.hxx>

#include <TopoDS_Shape.hxx>

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace RoboCrap3D {

OccPart::OccPart(const TopoDS_Shape& shape, const OccPartProps& props)
    : m_color(props.color),
      m_transform(props.transform),
      m_selectionMode(props.selectionMode),
      m_wireframe(props.wireframe),
      m_topmost(props.topmost),
      m_markerSize(props.markerSize),
      m_lineWidth(props.lineWidth)
{
  if (shape.IsNull()) return;

  createPresentation(shape);
  configureBasePresentation();
  applyColor();
  configureFaceBoundary();
  applyTransform();

  if (props.showTrihedron) {
    enableTrihedron(props.trihedronSize);
  }
}
bool OccPart::isValid() const
{
  return !m_handle.IsNull();
}

const Handle(AIS_Shape)& OccPart::handle() const
{
  return m_handle;
}

const Handle(AIS_Trihedron)& OccPart::trihedron() const
{
  return m_trihedron;
}

const Quantity_Color& OccPart::color() const
{
  return m_color;
}

void OccPart::setColor(const Quantity_Color& color)
{
  m_color = color;
  applyColor();
}

const gp_Trsf& OccPart::transform() const
{
  return m_transform;
}

void OccPart::setTransform(const gp_Trsf& transform)
{
  m_transform = transform;
  applyTransform();
}

OccSelectionMode OccPart::selectionMode() const
{
  return m_selectionMode;
}

void OccPart::setSelectionMode(const OccSelectionMode selectionMode)
{
  m_selectionMode = selectionMode;
}

bool OccPart::hasTrihedron() const
{
  return !m_trihedron.IsNull();
}

void OccPart::enableTrihedron(const double size)
{
  m_trihedronEnabled = true;
  m_trihedronSize = size > 0.0 ? size : 20.0;

  rebuildTrihedron();
}

void OccPart::disableTrihedron()
{
  m_trihedronEnabled = false;
  m_trihedron.Nullify();
}

void OccPart::createPresentation(const TopoDS_Shape& shape)
{
  if (shape.IsNull()) return;

  m_handle = new AIS_Shape(shape);
}

void OccPart::configureBasePresentation()
{
  if (m_handle.IsNull()) return;

  m_handle->SetDisplayMode(m_wireframe ? AIS_WireFrame : AIS_Shaded);
  m_handle->SetMaterial(Graphic3d_NOM_SATIN);
  if (m_markerSize > 0.0 || m_lineWidth > 0.0) {
    Handle(Prs3d_Drawer) drawer = m_handle->Attributes();
    if (drawer.IsNull()) {
      drawer = new Prs3d_Drawer();
      m_handle->SetAttributes(drawer);
    }
    if (m_markerSize > 0.0)
      drawer->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_BALL, m_color, m_markerSize));
    if (m_lineWidth > 0.0)
      drawer->SetLineAspect(new Prs3d_LineAspect(m_color, Aspect_TOL_SOLID, m_lineWidth));
  }
}

void OccPart::configureFaceBoundary()
{
  if (m_handle.IsNull()) return;

  Handle(Prs3d_Drawer) drawer = m_handle->Attributes();

  if (drawer.IsNull()) {
    drawer = new Prs3d_Drawer();
    m_handle->SetAttributes(drawer);
  }

  drawer->SetFaceBoundaryDraw(true);
  drawer->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.5));
}

void OccPart::applyColor()
{
  if (m_handle.IsNull())  return;

  m_handle->SetColor(m_color);
}

void OccPart::applyTransform()
{
  if (!m_handle.IsNull()) {
    m_handle->SetLocalTransformation(m_transform);
  }

  if (!m_trihedron.IsNull()) {
    m_trihedron->SetLocalTransformation(m_transform);
  }
}

void OccPart::rebuildTrihedron()
{
  if (!m_trihedronEnabled) {
    m_trihedron.Nullify();
    return;
  }

  const gp_Ax2 localPlacement(
      gp_Pnt(0.0, 0.0, 0.0),
      gp_Dir(0.0, 0.0, 1.0),
      gp_Dir(1.0, 0.0, 0.0)
      );

  Handle(Geom_Axis2Placement) axisPlacement = new Geom_Axis2Placement(localPlacement);

  m_trihedron = new AIS_Trihedron(axisPlacement);

  configureTrihedron();
  applyTransform();
}

void OccPart::configureTrihedron()
{
  if (m_trihedron.IsNull()) return;

  m_trihedron->SetSize(m_trihedronSize);
  m_trihedron->SetDatumDisplayMode(Prs3d_DM_Shaded);

  // Local frame without arrow heads.
  m_trihedron->SetDrawArrows(false);

  Handle(Prs3d_Drawer) drawer = m_trihedron->Attributes();

  if (drawer.IsNull()) {
    drawer = new Prs3d_Drawer();
    m_trihedron->SetAttributes(drawer);
  }

  drawer->SetLineAspect(new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.5));

  Handle(Prs3d_DatumAspect) datumAspect = drawer->DatumAspect();

  if (datumAspect.IsNull()) {
    datumAspect = new Prs3d_DatumAspect();
    drawer->SetDatumAspect(datumAspect);
  }

  // Local frame without X/Y/Z labels.
  datumAspect->SetDrawLabels(false);
  datumAspect->ShadingAspect(Prs3d_DP_XAxis)->SetColor(Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB));
  datumAspect->ShadingAspect(Prs3d_DP_YAxis)->SetColor(Quantity_Color(0.0, 0.8, 0.0, Quantity_TOC_RGB));
  datumAspect->ShadingAspect(Prs3d_DP_ZAxis)->SetColor(Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB));

  m_trihedron->SetToUpdate();
}

} // namespace RoboCrap3D
