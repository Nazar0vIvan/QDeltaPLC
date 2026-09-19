#include "utils.h"

#include <algorithm>
#include <cmath>

#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace RoboCrap3D {

namespace {

double clampSmall(double value, double eps = GeomConst::Eps)
{
  return std::abs(value) <= eps ? 0.0 : value;
}

M3d cleaned(const M3d& m)
{
  return M3d{
    clampSmall(m.Value(1, 1)), clampSmall(m.Value(1, 2)), clampSmall(m.Value(1, 3)),
    clampSmall(m.Value(2, 1)), clampSmall(m.Value(2, 2)), clampSmall(m.Value(2, 3)),
    clampSmall(m.Value(3, 1)), clampSmall(m.Value(3, 2)), clampSmall(m.Value(3, 3))
  };
}

} // namespace


Quantity_Color rgb(int r, int g, int b)
{
  return Quantity_Color(
      static_cast<double>(std::clamp(r, 0, 255)) / 255.0,
      static_cast<double>(std::clamp(g, 0, 255)) / 255.0,
      static_cast<double>(std::clamp(b, 0, 255)) / 255.0,
      Quantity_TOC_RGB
  );
}

std::optional<V3d> jsonValueToPoint(const QJsonValue& value)
{
  if (!value.isArray()) return std::nullopt;

  const QJsonArray array = value.toArray();

  if (array.size() != 3) {
    return std::nullopt;
  }

  if (!array[0].isDouble() || !array[1].isDouble() || !array[2].isDouble()) {
    return std::nullopt;
  }

  return V3d {
    array[0].toDouble(),
    array[1].toDouble(),
    array[2].toDouble()
  };
}

std::optional<QVector<V3d>> jsonArrayToPoints(const QJsonArray& array)
{
  QVector<V3d> points;
  points.reserve(array.size());

  for (const QJsonValue& value : array) {
    const auto point = jsonValueToPoint(value);

    if (!point.has_value()) return std::nullopt;

    points.push_back(*point);
  }

  return points;
}

bool nearlyEqual(double lhs, double rhs, double eps)
{
  return std::abs(lhs - rhs) <= eps;
}

bool isFinite(const V3d& v)
{
  return std::isfinite(v.X())
      && std::isfinite(v.Y())
      && std::isfinite(v.Z());
}

bool isFinite(const V4d& v)
{
  return std::isfinite(v[0])
      && std::isfinite(v[1])
      && std::isfinite(v[2])
      && std::isfinite(v[3]);
}

std::optional<V3d> normalize(const V3d& v, double eps)
{
  if (!isFinite(v)) return std::nullopt;

  const double len2 = v.SquareMagnitude();

  if (len2 <= eps * eps) return std::nullopt;

  return v.Multiplied(1.0 / std::sqrt(len2));
}

M4d makeTranslation(const V3d& delta)
{
  M4d result;
  result.SetTranslation(delta);
  return result;
}

M4d makeRotation(double angleDeg, const V3d& axis)
{
  const double angleRad = angleDeg * GeomConst::DegToRad;
  M4d result;
  result.SetRotation(gp_Ax1{gp_Pnt{0.0, 0.0, 0.0}, gp_Dir{axis.X(), axis.Y(), axis.Z()}}, angleRad);
  return result;
}

M4d makeTransform(const M3d& rot, const V3d& origin)
{
  M4d result;

  result.SetValues(
    rot.Value(1, 1), rot.Value(1, 2), rot.Value(1, 3), origin.X(),
    rot.Value(2, 1), rot.Value(2, 2), rot.Value(2, 3), origin.Y(),
    rot.Value(3, 1), rot.Value(3, 2), rot.Value(3, 3), origin.Z());

  return result;
}

bool isBasis(const V3d& v1, const V3d& v2, const V3d& v3, double eps)
{
  if (!isFinite(v1) || !isFinite(v2) || !isFinite(v3)) {
    return false;
  }

  const bool isUnit =
    nearlyEqual(v1.SquareMagnitude(), 1.0, eps) &&
    nearlyEqual(v2.SquareMagnitude(), 1.0, eps) &&
    nearlyEqual(v3.SquareMagnitude(), 1.0, eps);

  const bool isOrthogonal =
    nearlyEqual(v1.Dot(v2), 0.0, eps) &&
    nearlyEqual(v2.Dot(v3), 0.0, eps) &&
    nearlyEqual(v1.Dot(v3), 0.0, eps);

  const bool isRightHanded = nearlyEqual(v1.Crossed(v2).Dot(v3), 1.0, eps);

  return isUnit && isOrthogonal && isRightHanded;
}

std::optional<OrthoBasis> vecs2basis(const V3d& v1, const V3d& v2, const V3d& v3, double eps)
{
  const auto e1 = normalize(v1, eps);
  const auto e2 = normalize(v2, eps);
  const auto e3 = normalize(v3, eps);

  if (!e1.has_value() || !e2.has_value() || !e3.has_value()) {
    return std::nullopt;
  }

  if (!isBasis(*e1, *e2, *e3, eps)) {
    return std::nullopt;
  }

  return OrthoBasis{*e1, *e2, *e3};
}

M3d basis2rot(const OrthoBasis& orthobasis)
{
  return M3d {
    orthobasis.e1.X(), orthobasis.e2.X(), orthobasis.e3.X(),
    orthobasis.e1.Y(), orthobasis.e2.Y(), orthobasis.e3.Y(),
    orthobasis.e1.Z(), orthobasis.e2.Z(), orthobasis.e3.Z()
  };
}

EulerSolution rot2euler(const M3d& r)
{
  const double r20 = std::clamp(r.Value(3, 1), -1.0, 1.0);

  const double b1 = -std::asin(r20);
  const double cb = std::cos(b1);

  double a1 = 0.0;
  double a2 = 0.0;

  double b2 = b1;

  double c1 = 0.0;
  double c2 = 0.0;

  if (std::abs(cb) > GeomConst::Eps) {
    a1 = std::atan2(r.Value(2, 1), r.Value(1, 1));
    c1 = std::atan2(r.Value(3, 2), r.Value(3, 3));

    b2 = GeomConst::Pi - b1;
    a2 = std::atan2(-r.Value(2, 1), -r.Value(1, 1));
    c2 = std::atan2(-r.Value(3, 2), -r.Value(3, 3));
  } else {
    a1 = std::atan2(-r.Value(1, 2), r.Value(2, 2));
    a2 = a1;
  }

  constexpr double k = GeomConst::RadToDeg;

  return {
    k * a1, k * a2,
    k * b1, k * b2,
    k * c1, k * c2
  };
}

M3d euler2rot(double A, double B, double C)
{
  const M4d rz = makeRotation(A, V3d{0.0, 0.0, 1.0});
  const M4d ry = makeRotation(B, V3d{0.0, 1.0, 0.0});
  const M4d rx = makeRotation(C, V3d{1.0, 0.0, 0.0});

  const M4d transform = rz.Multiplied(ry).Multiplied(rx);

  return cleaned(transform.VectorialPart());
}

std::optional<V3d> prjPointOnLine(const V3d& linePoint, const V3d& lineDir, const V3d& point)
{
  if (!isFinite(linePoint) || !isFinite(lineDir) || !isFinite(point)) {
    return std::nullopt;
  }

  const double dirLen2 = lineDir.SquareMagnitude();

  if (dirLen2 <= GeomConst::Eps * GeomConst::Eps) {
    return std::nullopt;
  }

  const double t = point.Subtracted(linePoint).Dot(lineDir) / dirLen2;

  return linePoint.Added(lineDir.Multiplied(t));
}

std::optional<V3d> prjUnitOnPlane(const V3d& vec, const V3d& normal)
{
  if (!isFinite(vec) || !isFinite(normal)) {
    return std::nullopt;
  }

  const auto unitNormal = normalize(normal);

  if (!unitNormal.has_value()) {
    return std::nullopt;
  }

  const V3d projection = vec.Subtracted(unitNormal->Multiplied(vec.Dot(*unitNormal)));

  return normalize(projection);
}

std::optional<V3d> prjPointToPlane(const V3d& point, const V4d& planeCoeffs)
{
  if (!isFinite(point) || !isFinite(planeCoeffs)) {
    return std::nullopt;
  }

  const V3d normal{planeCoeffs[0], planeCoeffs[1], planeCoeffs[2]};

  const double normalLen2 = normal.SquareMagnitude();

  if (normalLen2 <= GeomConst::Eps * GeomConst::Eps) {
    return std::nullopt;
  }

  const double planeValue = normal.Dot(point) + planeCoeffs[3];

  return point.Subtracted(normal.Multiplied(planeValue / normalLen2));
}

std::optional<V3d> polyfit2d(const V3d& p0, const V3d& p1, const V3d& p2)
{
  if (!isFinite(p0) || !isFinite(p1) || !isFinite(p2)) {
    return std::nullopt;
  }

  const double x0 = p0.X();
  const double x1 = p1.X();
  const double x2 = p2.X();

  if (nearlyEqual(x0, x1) || nearlyEqual(x0, x2) || nearlyEqual(x1, x2)) {
    return std::nullopt;
  }

  const double y0 = p0.Y();
  const double y1 = p1.Y();
  const double y2 = p2.Y();

  const M3d lhs {
    x0 * x0, x0, 1.0,
    x1 * x1, x1, 1.0,
    x2 * x2, x2, 1.0
  };

  const double det = lhs.Determinant();

  if (std::abs(det) <= GeomConst::Eps) {
    return std::nullopt;
  }

  const M3d lhsA {
    y0, x0, 1.0,
    y1, x1, 1.0,
    y2, x2, 1.0
  };

  const M3d lhsB {
    x0 * x0, y0, 1.0,
    x1 * x1, y1, 1.0,
    x2 * x2, y2, 1.0
  };

  const M3d lhsC {
    x0 * x0, x0, y0,
    x1 * x1, x1, y1,
    x2 * x2, x2, y2
  };

  return V3d {
    lhsA.Determinant() / det,
    lhsB.Determinant() / det,
    lhsC.Determinant() / det
  };
}

V3d deriv2d(const V3d& point, const V3d& coeffs)
{
  if (!isFinite(point) || !isFinite(coeffs)) {
    return V3d{1.0, 0.0, 0.0};
  }

  const double a = coeffs.X();
  const double b = coeffs.Y();

  const double slope = 2.0 * a * point.X() + b;

  const auto unit = normalize(V3d{1.0, slope, 0.0});

  if (!unit.has_value()) {
    return V3d{1.0, 0.0, 0.0};
  }

  return *unit;
}

std::vector<double> acos2(const double value)
{
  if (value < -1.0 - GeomConst::Eps || value > 1.0 + GeomConst::Eps) {
    return {};
  }

  const double q = std::acos(std::clamp(value, -1.0, 1.0));

  if (std::abs(q) <= GeomConst::Eps || std::abs(std::abs(q) - GeomConst::Pi) <= GeomConst::Eps) {
    return {q};
  }

  return {q, -q};
}


bool isEqual(const V6d &lhs, const V6d &rhs)
{
  for (std::size_t i = 0; i < 6; ++i) {
    if (std::abs(lhs[i] - rhs[i]) > GeomConst::Eps)
      return false;
  }

  return true;
}

} // namespace RoboCrap3D
