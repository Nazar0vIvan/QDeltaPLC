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
  double rmsResidual = 0.0; // Fit error of the original centers before compensation.

  static std::optional<BoundedCylinder> fromPoints(const std::vector<Point>& points);
  // Applies JSON probe compensation to radius only, preserving samples and axis.
  static std::optional<BoundedCylinder> fromJsonFile(const QString& path);
};
