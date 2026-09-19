#pragma once

#include <Quantity_Color.hxx>

#include <QJsonArray>
#include <QJsonValue>
#include <QVector>

#include <optional>

#include "mathtypes.h"

namespace RoboCrap3D {

Quantity_Color rgb(int r, int g, int b);

std::optional<V3d> jsonValueToPoint(const QJsonValue& value);
std::optional<QVector<V3d>> jsonArrayToPoints(const QJsonArray& array);

bool nearlyEqual(double lhs, double rhs, double eps = GeomConst::Eps);

bool isFinite(const V3d& v);
bool isFinite(const V4d& v);

std::optional<V3d> normalize(const V3d& v, double eps = GeomConst::Eps);

M4d makeTranslation(const V3d& delta);
M4d makeRotation(double angleDeg, const V3d& axis);
M4d makeTransform(const M3d& rot, const V3d& origin);

bool isBasis(const V3d& v1, const V3d& v2, const V3d& v3, double eps = GeomConst::Eps);
std::optional<OrthoBasis> vecs2basis(const V3d& v1, const V3d& v2, const V3d& v3, double eps = GeomConst::Eps);
M3d basis2rot(const OrthoBasis& orthobasis);
EulerSolution rot2euler(const M3d& rot);
M3d euler2rot(double A, double B, double C);

std::optional<V3d> prjPointOnLine(const V3d& linePoint, const V3d& lineDir, const V3d& point);
std::optional<V3d> prjUnitOnPlane(const V3d& vec, const V3d& normal);
std::optional<V3d> prjPointToPlane(const V3d& point, const V4d& planeCoeffs);

std::optional<V3d> polyfit2d(const V3d& p0, const V3d& p1, const V3d& p2);
V3d deriv2d(const V3d& point,const V3d& coeffs);

bool isEqual(const V6d& lhs, const V6d& rhs);
std::vector<double> acos2(const double value);


} // namespace RoboCrap3D
