#include "occviewer.h"
#include "3d/math/utils.h"

#include <QDebug>

#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>

#include <Aspect_DisplayConnection.hxx>

#include <Graphic3d_Camera.hxx>
#include <Graphic3d_RenderingParams.hxx>
#include <Graphic3d_TypeOfShadingModel.hxx>
#include <Graphic3d_ZLayerId.hxx>

#include <OpenGl_GraphicDriver.hxx>

#include <Prs3d_Drawer.hxx>
#include <Prs3d_TypeOfHighlight.hxx>

#include <Quantity_Color.hxx>

#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#include <Standard_Failure.hxx>

#if defined(Q_OS_WIN)
  #include <WNT_Window.hxx>
#elif defined(Q_OS_LINUX)
  #include <Xw_Window.hxx>
#elif defined(Q_OS_MACOS)
  #include <Cocoa_Window.hxx>
#endif



namespace RoboCrap3D {

OccViewer::OccViewer(const Aspect_Handle nativeWindowHandle)
{
  initialize(nativeWindowHandle);
}

OccViewer::~OccViewer()
{
  try {
    if (!m_view.IsNull()) m_view->Remove();
  } catch (const Standard_Failure& failure) {
    qWarning() << "Cannot release OCCT view:" << failure.what();
  }
}

bool OccViewer::isValid() const
{
  return !m_displayConnection.IsNull()
      && !m_graphicDriver.IsNull()
      && !m_viewer.IsNull()
      && !m_context.IsNull()
      && !m_view.IsNull()
      && !m_window.IsNull();
}

const Handle(AIS_InteractiveContext)& OccViewer::context() const
{
  return m_context;
}

const Handle(V3d_View)& OccViewer::view() const
{
  return m_view;
}

void OccViewer::resize()
{
  if (m_view.IsNull()) return;

  m_view->MustBeResized();
}

void OccViewer::redraw()
{
  if (m_view.IsNull()) return;

  m_view->Redraw();
}

void OccViewer::updateCurrentViewer()
{
  if (m_context.IsNull()) return;

  m_context->UpdateCurrentViewer();
}

void OccViewer::fitAll()
{
  if (m_view.IsNull()) return;

  m_view->FitAll();
  m_view->ZFitAll();
}

void OccViewer::initialize(const Aspect_Handle nativeWindowHandle)
{
  if (!isNativeHandleValid(nativeWindowHandle)) {
    qWarning() << "Cannot initialize OCCT viewer: native window handle is null";
    return;
  }

  createViewerCore();

  if (m_viewer.IsNull() || m_context.IsNull()) {
    qWarning() << "Cannot initialize OCCT viewer: failed to create viewer core";
    return;
  }

  createView(nativeWindowHandle);

  if (!isValid()) {
    qWarning() << "Cannot initialize OCCT viewer: failed to create view/window";
    return;
  }

  configureViewer();
  configureContext();
  configureView();
  configureRenderingParams();
  configureHighlightStyles();

  resize();
}

void OccViewer::createViewerCore()
{
  m_displayConnection = new Aspect_DisplayConnection();
  m_graphicDriver = new OpenGl_GraphicDriver(m_displayConnection);

  m_viewer = new V3d_Viewer(m_graphicDriver);

  m_context = new AIS_InteractiveContext(m_viewer);
}

void OccViewer::createView(const Aspect_Handle nativeWindowHandle)
{
  if (m_viewer.IsNull()) return;

  m_view = m_viewer->CreateView();
  if (m_view.IsNull()) return;

#if defined(Q_OS_WIN)
  m_window = new WNT_Window(nativeWindowHandle);
#elif defined(Q_OS_LINUX)
  const auto xWindow = static_cast<Window>(reinterpret_cast<std::uintptr_t>(nativeWindowHandle));
  m_window = new Xw_Window(m_displayConnection, xWindow);
#elif defined(Q_OS_MACOS)
  m_window = new Cocoa_Window(nativeWindowHandle);
#else
#error Unsupported platform for OccViewer
#endif

  if (m_window.IsNull()) return;

  m_view->SetWindow(m_window);

  if (!m_window->IsMapped()) {
    m_window->Map();
  }
}

void OccViewer::configureViewer()
{
  if (m_viewer.IsNull()) return;

  m_viewer->SetDefaultLights();
  m_viewer->SetLightOn();
}

void OccViewer::configureContext()
{
  if (m_context.IsNull()) return;

  m_context->SetAutomaticHilight(true);
  m_context->SetPixelTolerance(5);
}

void OccViewer::configureView()
{
  if (m_view.IsNull()) return;

  m_view->SetImmediateUpdate(false);

  m_view->SetBackgroundColor(
    Quantity_Color(0.85, 0.85, 0.85, Quantity_TOC_RGB)
  );

  if (!m_view->Camera().IsNull()) {
    m_view->Camera()->SetProjectionType(
      Graphic3d_Camera::Projection_Orthographic
    );
  }
}

void OccViewer::configureRenderingParams()
{
  if (m_view.IsNull()) return;

  Graphic3d_RenderingParams& params = m_view->ChangeRenderingParams();

  params.NbMsaaSamples = 4;
  params.ShadingModel = Graphic3d_TypeOfShadingModel_Phong;
  params.LineFeather = 1.2f;
}

void OccViewer::configureHighlightStyles()
{
  if (m_context.IsNull()) return;

  Handle(Prs3d_Drawer) hoverStyle = m_context->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic);

  if (!hoverStyle.IsNull()) {
    hoverStyle->SetColor(rgb(102, 179, 204));
    hoverStyle->SetDisplayMode(AIS_Shaded);
    hoverStyle->SetFaceBoundaryDraw(false);
    hoverStyle->SetZLayer(Graphic3d_ZLayerId_Top);
  }

  Handle(Prs3d_Drawer) selectionStyle = m_context->HighlightStyle(Prs3d_TypeOfHighlight_LocalSelected);

  if (!selectionStyle.IsNull()) {
    selectionStyle->SetColor(rgb(0, 128, 255));
    selectionStyle->SetDisplayMode(AIS_Shaded);
    selectionStyle->SetFaceBoundaryDraw(false);
    selectionStyle->SetZLayer(Graphic3d_ZLayerId_Top);
  }
}

bool OccViewer::isNativeHandleValid(const Aspect_Handle nativeWindowHandle)
{
  return nativeWindowHandle != nullptr;
}

} // namespace RoboCrap3D
