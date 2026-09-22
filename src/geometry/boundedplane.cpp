#include "boundedplane.h"

#include "plane.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

BoundedPlane::Point coordinates(const V3d& value)
{
  return {value.x(), value.y(), value.z()};
}

} // namespace

std::optional<BoundedPlane> BoundedPlane::fromPoints(const std::vector<Point>& points)
{
  QVector<V3d> samples;
  samples.reserve(static_cast<qsizetype>(points.size()));
  for (const auto& point : points) samples.append(V3d{point[0], point[1], point[2]});
  const auto plane = Plane::fromPoints(samples);
  if (!plane) return std::nullopt;

  const V3d normal = plane->normal();
  const V3d reference = std::abs(normal.x()) < 0.9 ? V3d{1, 0, 0} : V3d{0, 1, 0};
  const auto tangent = prjUnitOnPlane(reference, normal);
  if (!tangent) return std::nullopt;
  const V3d u = *tangent;
  const auto bitangent = normalize(normal.cross(u));
  if (!bitangent) return std::nullopt;
  const V3d v = *bitangent;
  const auto anchor = prjPointToPlane(samples.front(), plane->coeffs);
  if (!anchor) return std::nullopt;

  double minU = std::numeric_limits<double>::infinity();
  double minV = minU;
  double maxU = -minU;
  double maxV = -minU;
  for (const V3d& point : samples) {
    // Tangential coordinates equal those of the orthogonal projection. Using
    // differences from the first sample avoids subtracting large plane offsets.
    const V3d delta = point - samples.front();
    const double x = delta.dot(u);
    const double y = delta.dot(v);
    if (!std::isfinite(x) || !std::isfinite(y)) return std::nullopt;
    minU = std::min(minU, x);
    maxU = std::max(maxU, x);
    minV = std::min(minV, y);
    maxV = std::max(maxV, y);
  }

  const double width = maxU - minU;
  const double height = maxV - minV;
  if (!std::isfinite(width) || !std::isfinite(height)
      || width <= GeomConst::Eps || height <= GeomConst::Eps) return std::nullopt;
  const V3d origin = *anchor + (minU + width / 2) * u + (minV + height / 2) * v;
  if (!origin.allFinite()) return std::nullopt;

  BoundedPlane result;
  for (int i = 0; i < 4; ++i) result.coefficients[i] = plane->coeffs[i];
  result.points = points;
  result.origin = coordinates(origin);
  result.axisU = coordinates(u);
  result.axisV = coordinates(v);
  result.width = width;
  result.height = height;
  return result;
}

std::optional<BoundedPlane> BoundedPlane::fromJsonFile(const QString& path)
{
  const auto samples = readJsonPoints(path);
  if (!samples) return std::nullopt;
  std::vector<Point> points;
  points.reserve(samples->size());
  for (const auto& sample : *samples) points.push_back(coordinates(sample));
  return fromPoints(points);
}
