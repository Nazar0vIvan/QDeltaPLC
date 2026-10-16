#pragma once

#include <AIS_ViewCube.hxx>

namespace RoboCrap3D {

class OccViewCube final
{
public:
  OccViewCube();

  bool isValid() const;

  const Handle(AIS_ViewCube)& handle() const;

private:
  void configureLabels();
  void configureStyle();
  void configureText();
  void configureEdgesAndCorners();
  void configureAxes();
  void configureTransformPersistence();
  void configureInteraction();

private:
  Handle(AIS_ViewCube) m_cube;
};

} // namespace RoboCrap3D
