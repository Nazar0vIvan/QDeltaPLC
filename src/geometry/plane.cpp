#include "geometry/plane.h"
#include "geometry/utils.h"

#include <Eigen/SVD>

#include <cmath>

std::optional<Plane> Plane::fromPoints(const QVector<V3d>& points, double eps)
{
  if (points.size() < 3 || !std::isfinite(eps) || eps < 0.0) return std::nullopt;

  Eigen::MatrixXd centered(points.size(), 3);
  const V3d anchor = points.front();

  for (int i = 0; i < points.size(); ++i) {
    const V3d& point = points[i];
    if (!point.allFinite()) return std::nullopt;

    centered.row(i) = (point - anchor).transpose();
  }

  // Center relative to a sample to avoid summing large world-coordinate offsets.
  const V3d meanOffset = centered.colwise().mean().transpose();
  const V3d centroid = anchor + meanOffset;
  centered.rowwise() -= meanOffset.transpose();
  if (!centroid.allFinite() || !centered.allFinite()) return std::nullopt;

  const Eigen::JacobiSVD<Eigen::MatrixXd, Eigen::ComputeFullV> svd(centered);
  if (svd.info() != Eigen::Success) return std::nullopt;

  const V3d singularValues = svd.singularValues();
  // Two independent directions determine a plane; its thickness may be zero.
  if (!singularValues.allFinite() || singularValues.x() <= 0.0
      || singularValues.y() <= eps * singularValues.x()) return std::nullopt;

  V3d normal = svd.matrixV().col(2);
  int dominantAxis = 0;
  for (int axis = 1; axis < 3; ++axis) {
    if (std::abs(normal[axis]) > std::abs(normal[dominantAxis])) dominantAxis = axis;
  }
  if (normal[dominantAxis] < 0.0) normal = -normal;

  const double d = -normal.dot(centroid);

  const V4d coeffs{normal.x(), normal.y(), normal.z(), d};

  if (!coeffs.allFinite()) {
    return std::nullopt;
  }

  return Plane{coeffs};
}

V3d Plane::normal() const noexcept
{
  return coeffs.head<3>();
}

