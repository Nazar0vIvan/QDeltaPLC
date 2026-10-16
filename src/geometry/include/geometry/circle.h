#pragma once

#include "mathtypes.h"

#include <QVector>
#include <optional>

// A validated 3D circumference. Samples retain their original coordinates/order.
class Circle
{
public:
  const V3d& center() const { return m_center; }
  const V3d& normal() const { return m_normal; }
  double radius() const { return m_radius; }
  double rmsResidual() const { return m_rmsResidual; }
  const QVector<V3d>& points() const { return m_points; }

  // Three non-collinear samples define a circumcircle; larger sets are fitted.
  static std::optional<Circle> fromPoints(QVector<V3d> points);

private:
  // Center, normal and radius define the circle; samples supply diagnostics.
  Circle(QVector<V3d> points, const V3d& center, const V3d& normal, double radius);

  QVector<V3d> m_points;
  V3d m_center = V3d::Zero();
  V3d m_normal = V3d::Zero();
  double m_radius = 0.0;
  double m_rmsResidual = 0.0;
};
