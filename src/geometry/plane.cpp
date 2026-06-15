#include "plane.h"

#include <cmath>

bool hasValidInput(
    const Eigen::Ref<const Eigen::VectorXd>& x,
    const Eigen::Ref<const Eigen::VectorXd>& y,
    const Eigen::Ref<const Eigen::VectorXd>& z)
{
  return x.size() == y.size() &&
         y.size() == z.size() &&
         x.size() >= 3 &&
         x.allFinite() &&
         y.allFinite() &&
         z.allFinite();
}

std::optional<Plane> Plane::fromPoints(
  const Eigen::Ref<const Eigen::VectorXd>& x,
  const Eigen::Ref<const Eigen::VectorXd>& y,
  const Eigen::Ref<const Eigen::VectorXd>& z,
  double eps)
{
  if (!hasValidInput(x, y, z)) {
    return std::nullopt;
  }

  const double n = static_cast<double>(x.size());

  M3d lhs;
  lhs << x.squaredNorm(), x.dot(y),        x.sum(),
         x.dot(y),        y.squaredNorm(), y.sum(),
         x.sum(),         y.sum(),         n;

  const V3d rhs{x.dot(z), y.dot(z), z.sum()};

  auto qr = lhs.colPivHouseholderQr();
  qr.setThreshold(eps);

  if (qr.rank() < 3) {
    return std::nullopt;
  }

  const V3d explicitCoeffs = qr.solve(rhs);

  if (!explicitCoeffs.allFinite()) {
    return std::nullopt;
  }

  const V3d normal = V3d{-explicitCoeffs.x(), -explicitCoeffs.y(), 1.0}.normalized();
  const double d = -explicitCoeffs.z() * normal.z();

  return Plane{V4d{normal.x(), normal.y(), normal.z(), d}};
}

V3d Plane::explicitCoeffs() const noexcept
{
  return {
    -coeffs.x() / coeffs.z(),
    -coeffs.y() / coeffs.z(),
    -coeffs.w() / coeffs.z()
  };
}