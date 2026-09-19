#pragma once

#include <AIS_Line.hxx>
#include <Geom_CartesianPoint.hxx>

namespace RoboCrap3D {

class OccWorldAxes final
{
public:
  explicit OccWorldAxes(double initialLength = 10.0);

  [[nodiscard]] bool isValid() const;

  [[nodiscard]] double length() const;
  void setLength(double length);

  [[nodiscard]] const Handle(AIS_Line)& xAxis() const;
  [[nodiscard]] const Handle(AIS_Line)& yAxis() const;
  [[nodiscard]] const Handle(AIS_Line)& zAxis() const;

private:
  void createAxes(double initialLength);
  void configureAxes();

private:
  double m_length = 10.0;

  Handle(Geom_CartesianPoint) m_originPoint;
  Handle(Geom_CartesianPoint) m_xEndPoint;
  Handle(Geom_CartesianPoint) m_yEndPoint;
  Handle(Geom_CartesianPoint) m_zEndPoint;

  Handle(AIS_Line) m_xAxis;
  Handle(AIS_Line) m_yAxis;
  Handle(AIS_Line) m_zAxis;
};

} // namespace RoboCrap3D
