#include "utils.h"

#include <algorithm>

bool nearlyEqual(double lhs, double rhs, double eps)
{
  return std::abs(lhs - rhs) <= eps;
}

std::optional<V3d> normalize(const V3d& v, double eps)
{
  const double len2 = v.squaredNorm();

  if (len2 <= eps * eps) return std::nullopt;

  return v / std::sqrt(len2);
}

V3d axisVec(const Axis axis, double value)
{
  switch (axis) {
    case Axis::X: return {value, 0.0, 0.0};
    case Axis::Y: return {0.0, value, 0.0};
    case Axis::Z: return {0.0, 0.0, value};
  }
}

M4d translation(const V3d& delta)
{
  M4d result = M4d::Identity();
  result.block<3, 1>(0, 3) = delta;
  return result;
}

M4d rotation(const double angleDeg, const Axis axis)
{
  const double angleRad = angleDeg * GeomConst::DegToRad;
  const double c = std::cos(angleRad);
  const double s = std::sin(angleRad);

  M4d R = M4d::Identity();

  switch (axis) {
    case Axis::X: {
      R(1, 1) = c; R(1, 2) = -s;
      R(2, 1) = s; R(2, 2) = c;
      break;
    }
    case Axis::Y: {
      R(0, 0) = c;  R(0, 2) = s;
      R(2, 0) = -s; R(2, 2) = c;
      break;
    }
    case Axis::Z: {
      R(0, 0) = c; R(0, 1) = -s;
      R(1, 0) = s; R(1, 1) = c;
      break;
    }
  }

  R = R.unaryExpr([](double v) { return std::abs(v) <= GeomConst::Eps ? 0.0 : v; });

  return R;
}

M4d transform(const M3d& rot, const V3d& origin)
{
  M4d T = M4d::Identity();

  T.block<3, 3>(0, 0) = rot;
  T.block<3, 1>(0, 3) = origin;

  return T;
}

bool isRotBasis(const Basis &basis, double eps)
{
  const V3d& x = basis.x;
  const V3d& y = basis.y;
  const V3d& z = basis.z;

  if (!x.allFinite() || !y.allFinite() || !z.allFinite()) {
    return false;
  }

  const bool isUnit =
      nearlyEqual(x.squaredNorm(), 1.0, eps) &&
      nearlyEqual(y.squaredNorm(), 1.0, eps) &&
      nearlyEqual(z.squaredNorm(), 1.0, eps);

  const bool isOrthogonal =
      nearlyEqual(x.dot(y), 0.0, eps) &&
      nearlyEqual(x.dot(z), 0.0, eps) &&
      nearlyEqual(y.dot(z), 0.0, eps);

  const bool isRightHanded =
      nearlyEqual(x.cross(y).dot(z), 1.0, eps);

  return isUnit && isOrthogonal && isRightHanded;
}

EulerSolution rot2euler(const M3d& r)
{
  const double r20 = std::clamp(r(2, 0), -1.0, 1.0);

  const double b1 = -std::asin(r20);
  const double cb = std::cos(b1);

  double a1 = 0.0; double a2 = 0.0;
  double b2 = b1;
  double c1 = 0.0; double c2 = 0.0;

  if (std::abs(cb) > GeomConst::Eps) {
    a1 = std::atan2(r(1, 0), r(0, 0));
    c1 = std::atan2(r(2, 1), r(2, 2));

    b2 = GeomConst::Pi - b1;
    a2 = std::atan2(-r(1, 0), -r(0, 0));
    c2 = std::atan2(-r(2, 1), -r(2, 2));
  } else {
    a1 = std::atan2(-r(0, 1), r(1, 1));
    a2 = a1;
  }

  constexpr double k = GeomConst::RadToDeg;

  return {
    k * a1, k * a2,
    k * b1, k * b2,
    k * c1, k * c2,
  };
}

M3d euler2rot(const double A, const double B, const double C)
{
  const double yaw = A * GeomConst::DegToRad;
  const double pitch = B * GeomConst::DegToRad;
  const double roll = C * GeomConst::DegToRad;

  const Eigen::AngleAxisd rotZ(yaw, V3d::UnitZ());
  const Eigen::AngleAxisd rotY(pitch, V3d::UnitY());
  const Eigen::AngleAxisd rotX(roll, V3d::UnitX());

  M3d R = (rotZ * rotY * rotX).toRotationMatrix();

  R = R.unaryExpr([](double v) { return std::abs(v) <= GeomConst::Eps ? 0.0 : v; });

  return R;
}

std::optional<M3d> basis2rot(const Basis &basis)
{
  const auto x = normalize(basis.x);
  const auto y = normalize(basis.y);
  const auto z = normalize(basis.z);

  if (!x || !y || !z) {
    return std::nullopt;
  }

  const Basis normalizedBasis{*x, *y, *z};

  if (!isRotBasis(normalizedBasis)) {
    return std::nullopt;
  }

  M3d R;
  R.col(0) = normalizedBasis.x;
  R.col(1) = normalizedBasis.y;
  R.col(2) = normalizedBasis.z;

  return R;
}

std::optional<V3d> prjPointOnLine(const V3d& linePoint, const V3d& lineDir, const V3d& point)
{
  const double dirLen2 = lineDir.squaredNorm();

  if (dirLen2 <= GeomConst::Eps * GeomConst::Eps) {
    return std::nullopt;
  }

  const double t = (point - linePoint).dot(lineDir) / dirLen2;
  return linePoint + t * lineDir;
}

std::optional<V3d> prjUnitOnPlane(const V3d& vec, const V3d& normal)
{
  const auto unitNormal = normalize(normal);

  if (!unitNormal) {
      return std::nullopt;
    }

  return normalize(vec - vec.dot(*unitNormal) * *unitNormal);
}

std::optional<V3d> polyfit2d(const V3d& p0, const V3d& p1, const V3d& p2)
{
  if (!p0.allFinite() || !p1.allFinite() || !p2.allFinite()) {
    return std::nullopt;
  }

  const double x0 = p0.x();
  const double x1 = p1.x();
  const double x2 = p2.x();

  if (nearlyEqual(x0, x1) || nearlyEqual(x0, x2) || nearlyEqual(x1, x2)) {
    return std::nullopt;
  }

  M3d lhs;
  lhs << x0 * x0, x0, 1.0,
         x1 * x1, x1, 1.0,
         x2 * x2, x2, 1.0;

  return lhs.colPivHouseholderQr().solve(V3d{p0.y(), p1.y(), p2.y()});
}

V3d deriv2d(const V3d& point, const V3d& coeffs)
{
  const double a = coeffs[0];
  const double b = coeffs[1];
  const double slope = 2.0 * a * point.x() + b;

  return V3d{1.0, slope, 0.0}.normalized();
}

