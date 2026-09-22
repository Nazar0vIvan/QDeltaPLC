#pragma once

#include <QString>

#include <array>
#include <optional>
#include <vector>

// Fitted numerical data. Points remain in their original order and coordinates.
struct BoundedCylinder
{
  using Point = std::array<double, 3>;
  std::vector<Point> points;
  Point origin{}; // Midpoint of the sampled axial interval, on the fitted axis.
  Point axis{};   // Unit direction; dominant component is positive.
  double radius = 0.0;
  double length = 0.0;
  double rmsResidual = 0.0;

  static std::optional<BoundedCylinder> fromPoints(const std::vector<Point>& points);
  static std::optional<BoundedCylinder> fromJsonFile(const QString& path);
};
