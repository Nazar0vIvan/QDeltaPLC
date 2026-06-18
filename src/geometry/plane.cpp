#include "plane.h"
#include "utils.h"

#include <cmath>

namespace {

bool hasValidInput(const VXdRef& x, const VXdRef& y, const VXdRef& z)
{
  return x.size() == y.size() &&
         y.size() == z.size() &&
         x.size() >= 3 &&
         x.allFinite() &&
         y.allFinite() &&
         z.allFinite();
}

} // namespace

std::optional<Plane> Plane::fromPoints(const QVector<V3d>& points, double eps)
{
  if (points.size() < 3) return std::nullopt;

  VXd x(points.size());
  VXd y(points.size());
  VXd z(points.size());

  for (int i = 0; i < points.size(); ++i) {
    const V3d& point = points[i];
    if (!point.allFinite()) return std::nullopt;

    x(i) = point.x();
    y(i) = point.y();
    z(i) = point.z();
  }

  const double pointCount = static_cast<double>(points.size());

  M3d lhs;
  lhs << x.squaredNorm(), x.dot(y),        x.sum(),
         x.dot(y),        y.squaredNorm(), y.sum(),
         x.sum(),         y.sum(),         pointCount;

  const V3d rhs{x.dot(z), y.dot(z), z.sum()};

  auto qr = lhs.colPivHouseholderQr();
  qr.setThreshold(eps);

  if (qr.rank() < 3) return std::nullopt;

  const V3d explicitCoeffs = qr.solve(rhs);

  if (!explicitCoeffs.allFinite()) {
    return std::nullopt;
  }

  const V3d normal = V3d{-explicitCoeffs.x(), -explicitCoeffs.y(), 1.0}.normalized();

  const double d = -explicitCoeffs.z() * normal.z();

  const V4d coeffs{normal.x(), normal.y(), normal.z(), d};

  if (!coeffs.allFinite() || std::abs(coeffs.z()) <= eps) {
    return std::nullopt;
  }

  return Plane{coeffs};
}

std::optional<Plane> Plane::fromJsonFile(const QString &jsonFilePath, double eps)
{
  if (jsonFilePath.isEmpty()) {
    return std::nullopt;
  }

  QFile file(jsonFilePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return std::nullopt;
  }

  const QByteArray bytes = file.readAll();
  if (bytes.isEmpty()) {
    return std::nullopt;
  }

  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    return std::nullopt;
  }
  if (!document.isArray()) {
    return std::nullopt;
  }

  const auto points = jsonArrayToPoints(document.array());
  if (!points) {
    return std::nullopt;
  }

  return Plane::fromPoints(*points, eps);
}

V3d Plane::normal() const noexcept
{
  return coeffs.head<3>();
}

std::optional<V3d> Plane::explicitCoeffs(double eps) const noexcept
{
  if (!coeffs.allFinite() || std::abs(coeffs.z()) <= eps) {
    return std::nullopt;
  }

  return V3d{
    -coeffs.x() / coeffs.z(),
    -coeffs.y() / coeffs.z(),
    -coeffs.w() / coeffs.z()
  };
}

