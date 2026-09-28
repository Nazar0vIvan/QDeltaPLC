#pragma once

#include "mathtypes.h"

#include <QString>
#include <QVector>
#include <array>
#include <optional>

struct ProbeSamples;

// Validated numerical data; only factories construct it. Samples retain input order.
class BoundedPlane
{
public:
  using Point = std::array<double, 3>;
  const std::array<double, 4>& coefficients() const { return m_coefficients; }
  const QVector<V3d>& points() const { return m_points; }
  const Point& origin() const { return m_origin; }
  const Point& axisU() const { return m_axisU; }
  const Point& axisV() const { return m_axisV; }
  double width() const { return m_width; }
  double height() const { return m_height; }

  static std::optional<BoundedPlane> fromPoints(QVector<V3d> points);
  static std::optional<BoundedPlane> fromSamples(ProbeSamples samples);
  static std::optional<BoundedPlane> fromJsonFile(const QString& path);

private:
  // A fitted plane consists of samples, coefficients, its tangent frame and bounds.
  BoundedPlane(QVector<V3d> points, const std::array<double, 4>& coefficients,
               const std::array<Point, 3>& frame, const std::array<double, 2>& bounds);
  bool applyProbe(double radius, int dir);
  std::array<double, 4> m_coefficients{};
  QVector<V3d> m_points{};
  Point m_origin{};
  Point m_axisU{};
  Point m_axisV{};
  double m_width{};
  double m_height{};
};
