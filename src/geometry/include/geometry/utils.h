#pragma once

#include <cmath>
#include <optional>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonArray>

#include "mathtypes.h"


struct ProbeSamples {
  QVector<V3d> points;
  double radius = 0.0;
  int dir = 1;
};

// Legacy arrays have zero compensation; objects require points, radius and dir.
std::optional<ProbeSamples> decodeProbeSamples(const QJsonDocument& document);

bool nearlyEqual(double lhs, double rhs, double eps = GeomConst::Eps);
std::optional<V3d> normalize(const V3d& v, double eps = GeomConst::Eps);

M4d makeTranslation(const V3d& delta);
M4d makeRotation(const double angleDeg, const Axis axis);
// Finite angle and finite nonzero axis; numerical callers validate their inputs.
// Unlike the principal-axis overload, this retains small rotation components.
M4d makeRotation(double angleDeg, const V3d& axis);
M4d makeTransform(const M3d& rot, const V3d& origin);
// Inverse of a rigid transform: rotation must be orthonormal, with no scale/shear.
M4d inverseRigidTransform(const M4d& transform);

bool isBasis(const V3d& v1, const V3d& v2, const V3d& v3, double eps = GeomConst::Eps);
std::optional<OrthoBasis> vecs2basis(const V3d& v1, const V3d& v2, const V3d& v3, double eps = GeomConst::Eps);
M3d basis2rot(const OrthoBasis& orthobasis);
EulerSolution rot2euler(const M3d& rot);
M3d euler2rot(const double A, const double B, const double C);

std::optional<V3d> prjUnitOnPlane(const V3d& vec, const V3d& normal);
std::optional<V3d> prjPointToPlane(const V3d& point, const V4d& planeCoeffs);
