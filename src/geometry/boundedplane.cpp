#include "boundedplane.h"

#include "plane.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

BoundedPlane::Point coordinates(const V3d& value)
{
  return {value.x(), value.y(), value.z()};
}

} // namespace

std::optional<BoundedPlane> BoundedPlane::fromPoints(QVector<V3d> points)
{
  const auto& samples = std::as_const(points);
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

  const std::array<double, 4> coefficients{plane->coeffs[0], plane->coeffs[1], plane->coeffs[2], plane->coeffs[3]};
  return BoundedPlane(std::move(points), coefficients,
                       {coordinates(origin), coordinates(u), coordinates(v)}, {width, height});
}

std::optional<BoundedPlane> BoundedPlane::fromJsonFile(const QString& path)
{
  auto samples = readProbeSamples(path);
  return samples ? fromSamples(std::move(*samples)) : std::nullopt;
}

std::optional<BoundedPlane> BoundedPlane::fromSamples(ProbeSamples samples)
{
  if (!std::isfinite(samples.radius) || samples.radius < 0.0
      || (samples.dir != 1 && samples.dir != -1)) return std::nullopt;
  auto plane = fromPoints(std::move(samples.points));
  if (!plane) return std::nullopt;
  if (!plane->applyProbe(samples.radius, samples.dir)) return std::nullopt;
  return plane;
}

BoundedPlane::BoundedPlane(QVector<V3d> points, const std::array<double, 4>& coefficients,
                           const std::array<Point, 3>& frame, const std::array<double, 2>& bounds)
  : m_coefficients(coefficients), m_points(std::move(points)), m_origin(frame[0]), m_axisU(frame[1]),
    m_axisV(frame[2]), m_width(bounds[0]), m_height(bounds[1])
{}

bool BoundedPlane::applyProbe(double radius, int dir)
{
  const double shift = dir * radius;
  m_coefficients[3] -= shift;
  if (!std::isfinite(m_coefficients[3])) return false;
  for (int i = 0; i < 3; ++i) {
    m_origin[i] += shift * m_coefficients[i];
    if (!std::isfinite(m_origin[i])) return false;
  }
  return true;
}
