#pragma once

#include <cmath>
#include <optional>

#include "mathtypes.h"

bool nearlyEqual(double lhs, double rhs, double eps = GeomConst::Eps);
std::optional<V3d> normalize(const V3d& v, double eps = GeomConst::Eps);
V3d axisVec(const Axis axis, double value);

M4d makeTranslation(const V3d& delta);
M4d makeRotation(const double angleDeg, const Axis axis);
M4d makeTransform(const M3d& rot, const V3d& origin);

bool isBasis(const V3d& v1, const V3d& v2, const V3d& v3, double eps = GeomConst::Eps);
std::optional<OrthoBasis> vecs2basis(const V3d& v1, const V3d& v2, const V3d& v3, double eps = GeomConst::Eps);
M3d basis2rot(const OrthoBasis& orthobasis);
EulerSolution rot2euler(const M3d& rot);
M3d euler2rot(const double A, const double B, const double C);

std::optional<V3d> prjPointOnLine(const V3d& linePoint, const V3d& lineDir, const V3d& point);
std::optional<V3d> prjUnitOnPlane(const V3d& vec, const V3d& normal);
std::optional<V3d> prjPointToPlane(const V3d& point, const V4d& planeCoeffs);

std::optional<V3d> polyfit2d(const V3d& p0, const V3d& p1, const V3d& p2);
V3d deriv2d(const V3d& point, const V3d& coeffs);
