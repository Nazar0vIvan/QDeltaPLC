#pragma once

#include <Aspect_Handle.hxx>
#include <Standard_Handle.hxx>

class AIS_InteractiveContext;
class Aspect_DisplayConnection;
class Aspect_Window;
class OpenGl_GraphicDriver;
class V3d_View;
class V3d_Viewer;

namespace RoboCrap3D {

class OccViewer final
{
public:
  explicit OccViewer(Aspect_Handle nativeWindowHandle);

  ~OccViewer();

  OccViewer(const OccViewer&) = delete;
  OccViewer& operator=(const OccViewer&) = delete;

  OccViewer(OccViewer&&) noexcept = delete;
  OccViewer& operator=(OccViewer&&) noexcept = delete;

  [[nodiscard]] bool isValid() const;

  [[nodiscard]] const Handle(AIS_InteractiveContext)& context() const;
  [[nodiscard]] const Handle(V3d_View)& view() const;

  void resize();
  void redraw();
  void updateCurrentViewer();
  void fitAll();

private:
  void initialize(Aspect_Handle nativeWindowHandle);

  void createViewerCore();
  void createView(Aspect_Handle nativeWindowHandle);

  void configureViewer();
  void configureContext();
  void configureView();
  void configureRenderingParams();
  void configureHighlightStyles();

  [[nodiscard]] static bool isNativeHandleValid(Aspect_Handle nativeWindowHandle);

private:
  Handle(Aspect_DisplayConnection) m_displayConnection;
  Handle(OpenGl_GraphicDriver) m_graphicDriver;
  Handle(V3d_Viewer) m_viewer;
  Handle(AIS_InteractiveContext) m_context;
  Handle(V3d_View) m_view;
  Handle(Aspect_Window) m_window;
};

} // namespace RoboCrap3D
