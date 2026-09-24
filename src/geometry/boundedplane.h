#pragma once

#include <QString>

#include <array>
#include <optional>
#include <vector>

// Numerical data only. Original samples remain unprojected and in input order.
struct BoundedPlane
{
  using Point = std::array<double, 3>;
  std::array<double, 4> coefficients{};
  std::vector<Point> points;
  Point origin{}; // Rectangle center, on the plane after any probe compensation.
  Point axisU{};
  Point axisV{}; // U cross V = normal.
  double width = 0.0;
  double height = 0.0;

  static std::optional<BoundedPlane> fromPoints(const std::vector<Point>& points);
  // Applies JSON probe compensation to the plane and origin, preserving samples.
  static std::optional<BoundedPlane> fromJsonFile(const QString& path);
};
