#pragma once

#include <cmath>
#include <optional>

#include "mathtypes.h"

namespace GeomConst {
  constexpr double Eps = 1e-9;
  constexpr double Pi = 3.141592653589793238462643383279502884;
  constexpr double DegToRad = Pi / 180.0;
  constexpr double RadToDeg = 180.0 / Pi;
}

bool nearlyEqual(double lhs, double rhs, double eps = GeomConst::Eps);
std::optional<V3d> normalize(const V3d& v, double eps = GeomConst::Eps);
V3d axisVec(const Axis axis, double value);

M4d translation(const V3d& delta);
M4d rotation(const double angleDeg, const Axis axis);
M4d transform(const M3d& rot, const V3d& origin);

EulerSolution rot2euler(const M3d& rot);
M3d euler2rot(const double A, const double B, const double C);
std::optional<M3d> vec2rot(const V3d& ux, const V3d& uy, const V3d& uz);

std::optional<V3d> prjPointOnLine(const V3d& linePoint, const V3d& lineDir, const V3d& point);

std::optional<V3d> prjUnitOnPlane(const V3d& vec, const V3d& normal);

std::optional<V3d> polyfit2d(const V3d& p0, const V3d& p1, const V3d& p2);

V3d deriv2d(const V3d& point, const V3d& coeffs);