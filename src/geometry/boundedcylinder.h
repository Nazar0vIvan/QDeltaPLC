#pragma once

#include "mathtypes.h"

#include <QString>
#include <QVector>
#include <array>
#include <optional>

struct ProbeSamples;

// Validated numerical data; only factories construct it. Samples retain input order.
class BoundedCylinder
{
public:
  using Point = std::array<double, 3>;
  const QVector<V3d>& points() const { return m_points; }
  const Point& origin() const { return m_origin; }
  const Point& axis() const { return m_axis; }
  double radius() const { return m_radius; }
  double length() const { return m_length; }
  double rmsResidual() const { return m_rmsResidual; }

  static std::optional<BoundedCylinder> fromPoints(QVector<V3d> points);
  static std::optional<BoundedCylinder> fromSamples(ProbeSamples samples);
  static std::optional<BoundedCylinder> fromJsonFile(const QString& path);

private:
  // Samples, axis frame, radius/length and fit residual define a fitted cylinder.
	BoundedCylinder(QVector<V3d> points, const std::array<Point, 2>& frame, const std::array<double, 2>& dimensions, double residual);
  bool applyProbe(double radius, int dir);
  QVector<V3d> m_points{};
  Point m_origin{};
  Point m_axis{};
  double m_radius{};
  double m_length{};
  double m_rmsResidual{};
};
