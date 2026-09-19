#include "occviewcube.h"
#include "3d/math/utils.h"

#include <AIS_DisplayMode.hxx>
#include <AIS_ViewCube.hxx>

#include <Aspect_TypeOfTriedronPosition.hxx>

#include <Graphic3d_TransformPers.hxx>
#include <Graphic3d_TypeOfShadingModel.hxx>
#include <NCollection_Vec2.hxx>
#include <Graphic3d_ZLayerId.hxx>

#include <Prs3d_DatumAspect.hxx>
#include <Prs3d_DatumParts.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_ShadingAspect.hxx>

#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>

#include <V3d_TypeOfOrientation.hxx>

namespace RoboCrap3D {

OccViewCube::OccViewCube() : m_cube(new AIS_ViewCube())
{
  configureLabels();
  configureStyle();
  configureText();
  configureEdgesAndCorners();
  configureAxes();
  configureTransformPersistence();
  configureInteraction();
}

bool OccViewCube::isValid() const
{
  return !m_cube.IsNull();
}

const Handle(AIS_ViewCube)& OccViewCube::handle() const
{
  return m_cube;
}

void OccViewCube::configureLabels()
{
  if (m_cube.IsNull()) return;

  m_cube->SetBoxSideLabel(V3d_Zpos, "TOP");
  m_cube->SetBoxSideLabel(V3d_Zneg, "BOTTOM");

  m_cube->SetBoxSideLabel(V3d_Yneg, "FRONT");
  m_cube->SetBoxSideLabel(V3d_Ypos, "BACK");

  m_cube->SetBoxSideLabel(V3d_Xpos, "RIGHT");
  m_cube->SetBoxSideLabel(V3d_Xneg, "LEFT");
}

void OccViewCube::configureStyle()
{
  if (m_cube.IsNull()) return;

  m_cube->SetSize(50.0, false);

  m_cube->SetBoxColor(rgb(200, 200, 230));
  m_cube->SetInnerColor(rgb(0, 0, 0));
  m_cube->SetBoxTransparency(0.0);

  if (!m_cube->Attributes().IsNull()) {
    m_cube->Attributes()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
  }

  if (!m_cube->BoxSideStyle().IsNull() && !m_cube->BoxSideStyle()->Aspect().IsNull()) {
    m_cube->BoxSideStyle()->Aspect()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
  }

  if (!m_cube->BoxEdgeStyle().IsNull() && !m_cube->BoxEdgeStyle()->Aspect().IsNull()) {
    m_cube->BoxEdgeStyle()->Aspect()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
  }

  if (!m_cube->BoxCornerStyle().IsNull() && !m_cube->BoxCornerStyle()->Aspect().IsNull()) {
    m_cube->BoxCornerStyle()->Aspect()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
  }

  Handle(Prs3d_Drawer) hoverStyle = m_cube->DynamicHilightAttributes();

  if (!hoverStyle.IsNull()) {
    const Quantity_Color hoverColor = rgb(102, 179, 204);

    hoverStyle->SetColor(hoverColor);
    hoverStyle->SetDisplayMode(AIS_Shaded);
    hoverStyle->SetFaceBoundaryDraw(false);
    hoverStyle->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);

    if (!hoverStyle->ShadingAspect().IsNull()) {
      hoverStyle->ShadingAspect()->SetColor(hoverColor);

      if (!hoverStyle->ShadingAspect()->Aspect().IsNull()) {
        hoverStyle->ShadingAspect()->Aspect()->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
      }
    }
  }
}

void OccViewCube::configureText()
{
  if (m_cube.IsNull()) return;

  m_cube->SetTextColor(Quantity_NOC_BLACK);
  m_cube->SetFont("Arial");
  m_cube->SetFontHeight(14.0);
}

void OccViewCube::configureEdgesAndCorners()
{
  if (m_cube.IsNull()) return;

  m_cube->SetDrawEdges(true);
  m_cube->SetDrawVertices(true);
  m_cube->SetBoxFacetExtension(12.0);

  if (!m_cube->BoxEdgeStyle().IsNull()) {
    m_cube->BoxEdgeStyle()->SetColor(rgb(150, 150, 180));
  }

  if (!m_cube->BoxCornerStyle().IsNull()) {
    m_cube->BoxCornerStyle()->SetColor(rgb(120, 120, 150));
  }
}

void OccViewCube::configureAxes()
{
  if (m_cube.IsNull()) return;

  m_cube->SetDrawAxes(true);
  m_cube->SetAxesLabels("", "", "");

  m_cube->SetAxesPadding(10.0);
  m_cube->SetAxesRadius(1.0);
  m_cube->SetAxesConeRadius(0.0);
  m_cube->SetAxesSphereRadius(0.0);

  Handle(Prs3d_Drawer) drawer = m_cube->Attributes();

  if (drawer.IsNull()) {
    drawer = new Prs3d_Drawer();
    m_cube->SetAttributes(drawer);
  }

  Handle(Prs3d_DatumAspect) datumAspect = drawer->DatumAspect();

  if (datumAspect.IsNull()) {
    datumAspect = new Prs3d_DatumAspect();
    drawer->SetDatumAspect(datumAspect);
  }

  datumAspect->ShadingAspect(Prs3d_DP_XAxis)->SetColor(rgb(255, 0, 0));
  datumAspect->ShadingAspect(Prs3d_DP_YAxis)->SetColor(rgb(0, 255, 0));
  datumAspect->ShadingAspect(Prs3d_DP_ZAxis)->SetColor(rgb(0, 0, 255));
}

void OccViewCube::configureTransformPersistence()
{
  if (m_cube.IsNull()) return;

  m_cube->SetTransformPersistence(
    new Graphic3d_TransformPers(
        Graphic3d_TMF_TriedronPers,
        Aspect_TOTP_LEFT_LOWER,
        NCollection_Vec2<int>(100, 100)
    )
  );

  m_cube->SetZLayer(Graphic3d_ZLayerId_Topmost);
}

void OccViewCube::configureInteraction()
{
  if (m_cube.IsNull()) return;

  m_cube->SetResetCamera(true);
  m_cube->SetFitSelected(false);
}

} // namespace RoboCrap3D
