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
  Point origin{}; // Rectangle center, on the fitted plane.
  Point axisU{};
  Point axisV{}; // U cross V = normal.
  double width = 0.0;
  double height = 0.0;

  static std::optional<BoundedPlane> fromPoints(const std::vector<Point>& points);
  static std::optional<BoundedPlane> fromJsonFile(const QString& path);
};
