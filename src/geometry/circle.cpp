#include "circle.h"

#include "plane.h"
#include "utils.h"

#include <Eigen/Cholesky>
#include <Eigen/SVD>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace {

using Matrix6 = Eigen::Matrix<double, 6, 6>;

struct Fit {
  V3d center = V3d::Zero();
  V3d normal = V3d::Zero();
  double radius = 0.0;
};

struct Evaluation {
  Matrix6 hessian = Matrix6::Zero();
  V6d gradient = V6d::Zero();
  double cost = 0.0;
};

std::optional<std::array<V3d, 2>> tangentBasis(const V3d& normal)
{
  const V3d reference = std::abs(normal.x()) < 0.9 ? V3d{1, 0, 0} : V3d{0, 1, 0};
  const auto u = normalize(reference - reference.dot(normal) * normal);
  if (!u) return std::nullopt;
  return std::array<V3d, 2>{*u, normal.cross(*u)};
}

std::optional<Eigen::Vector2d> residual(const V3d& point, const Fit& fit)
{
  const V3d delta = point - fit.center;
  const double axial = delta.dot(fit.normal);
  const double radial = (delta - axial * fit.normal).stableNorm();
  const Eigen::Vector2d values{axial, radial - fit.radius};
  return values.allFinite() ? std::optional<Eigen::Vector2d>{values} : std::nullopt;
}

std::optional<double> cost(const QVector<V3d>& points, const Fit& fit)
{
  double result = 0.0;
  for (const V3d& point : points) {
    const auto values = residual(point, fit);
    if (!values) return std::nullopt;
    result += values->squaredNorm();
  }
  result /= static_cast<double>(points.size());
  return std::isfinite(result) ? std::optional<double>{result} : std::nullopt;
}

// Center XYZ, two normal tangent increments and radius are the six parameters.
std::optional<Evaluation> evaluate(const QVector<V3d>& points, const Fit& fit)
{
  const auto basis = tangentBasis(fit.normal);
  if (!basis) return std::nullopt;
  Evaluation result;
  for (const V3d& point : points) {
    const V3d delta = point - fit.center;
    const double axial = delta.dot(fit.normal);
    const V3d radialVector = delta - axial * fit.normal;
    const double radial = radialVector.stableNorm();
    if (!std::isfinite(radial) || radial <= GeomConst::Eps) return std::nullopt;
    const double radialError = radial - fit.radius;
    V6d axialDerivative = V6d::Zero();
    V6d radialDerivative = V6d::Zero();
    axialDerivative.head<3>() = -fit.normal;
    radialDerivative.head<3>() = -radialVector / radial;
    for (int i = 0; i < 2; ++i) {
      axialDerivative[3 + i] = delta.dot((*basis)[i]);
      radialDerivative[3 + i] = -axial * axialDerivative[3 + i] / radial;
    }
    radialDerivative[5] = -1.0;
    result.hessian += axialDerivative * axialDerivative.transpose()
                    + radialDerivative * radialDerivative.transpose();
    result.gradient += axial * axialDerivative + radialError * radialDerivative;
    result.cost += axial * axial + radialError * radialError;
  }
  const double count = static_cast<double>(points.size());
  result.hessian /= count;
  result.gradient /= count;
  result.cost /= count;
  if (!result.hessian.allFinite() || !result.gradient.allFinite()
      || !std::isfinite(result.cost)) return std::nullopt;
  return result;
}

std::optional<Fit> displaced(const Fit& fit, const V6d& change)
{
  const auto basis = tangentBasis(fit.normal);
  if (!basis) return std::nullopt;
  const auto normal = normalize(fit.normal + change[3] * (*basis)[0] + change[4] * (*basis)[1]);
  if (!normal) return std::nullopt;
  Fit next{fit.center + change.head<3>(), *normal, fit.radius + change[5]};
  if (!next.center.allFinite() || !std::isfinite(next.radius)
      || next.radius <= GeomConst::Eps) return std::nullopt;
  return next;
}

std::optional<Fit> circumcircle(const QVector<V3d>& points)
{
  const V3d a = points[1] - points[0];
  const V3d b = points[2] - points[0];
  const V3d cross = a.cross(b);
  const double squared = cross.squaredNorm();
	if (squared <= GeomConst::Eps * GeomConst::Eps * a.squaredNorm() * b.squaredNorm() || squared == 0.0) return std::nullopt;
  Fit fit;
	fit.center = points[0] + (a.squaredNorm() * b.cross(cross) + b.squaredNorm() * cross.cross(a)) / (2.0 * squared);
  fit.normal = cross / std::sqrt(squared);
  fit.radius = (points[0] - fit.center).stableNorm();
	if (!fit.center.allFinite() || !fit.normal.allFinite() || !std::isfinite(fit.radius) || fit.radius <= GeomConst::Eps) return std::nullopt;
  return fit;
}

std::optional<Fit> initialCircle(const QVector<V3d>& points)
{
  const auto plane = Plane::fromPoints(points);
  if (!plane) return std::nullopt;
  const V3d normal = plane->normal();
  const auto basis = tangentBasis(normal);
  if (!basis) return std::nullopt;
  const V3d origin = -plane->coeffs[3] * normal;
  Eigen::MatrixXd system(points.size(), 3);
  Eigen::VectorXd squaredRadii(points.size());
  for (qsizetype i = 0; i < points.size(); ++i) {
    const V3d delta = points[i] - origin;
    const double x = delta.dot((*basis)[0]);
    const double y = delta.dot((*basis)[1]);
    system.row(i) << 2.0 * x, 2.0 * y, 1.0;
    squaredRadii[i] = x * x + y * y;
  }
  Eigen::JacobiSVD<Eigen::MatrixXd, Eigen::ComputeThinU | Eigen::ComputeThinV> solver(system);
  solver.setThreshold(GeomConst::Eps);
  if (solver.info() != Eigen::Success || solver.rank() != 3) return std::nullopt;
  const V3d solution = solver.solve(squaredRadii);
  const double squaredRadius = solution[2] + solution.head<2>().squaredNorm();
  if (!solution.allFinite() || !std::isfinite(squaredRadius) || squaredRadius <= GeomConst::Eps * GeomConst::Eps) return std::nullopt;
  return Fit{origin + solution[0] * (*basis)[0] + solution[1] * (*basis)[1],
             normal, std::sqrt(squaredRadius)};
}

std::optional<Fit> refine(const QVector<V3d>& points, Fit fit)
{
  double damping = 1e-3;
  for (int iteration = 0; iteration < 200; ++iteration) {
    const auto current = evaluate(points, fit);
    if (!current) return std::nullopt;
    if (current->gradient.lpNorm<Eigen::Infinity>() <= 1e-10) return fit;
    bool improved = false;
    for (int trial = 0; trial < 12; ++trial) {
      Matrix6 regularized = current->hessian;
      for (int i = 0; i < 6; ++i)
        regularized(i, i) += damping * std::max(1.0, current->hessian(i, i));
      const V6d change = regularized.ldlt().solve(-current->gradient);
      if (!change.allFinite()) return std::nullopt;
      const auto candidate = displaced(fit, change);
      const auto candidateCost = candidate ? cost(points, *candidate) : std::nullopt;
      if (candidateCost && *candidateCost < current->cost) {
        fit = *candidate;
        damping = std::max(1e-12, damping / 3.0);
        improved = true;
        if (current->cost - *candidateCost <= 1e-12 * std::max(1.0, current->cost)) return fit;
        break;
      }
      damping *= 10.0;
    }
    if (!improved) return std::nullopt;
  }
  return std::nullopt;
}

bool wellConstrained(const QVector<V3d>& points, const Fit& fit)
{
  const auto evaluation = evaluate(points, fit);
  if (!evaluation) return false;
  const Eigen::JacobiSVD<Matrix6> solver(evaluation->hessian);
  const auto singular = solver.singularValues();
  return solver.info() == Eigen::Success && singular.allFinite() && singular[0] > 0.0
      && singular[5] > 1e-12 * singular[0];
}

} // namespace

std::optional<Circle> Circle::fromPoints(QVector<V3d> points)
{
  const auto& samples = std::as_const(points);
  if (samples.size() < 3) return std::nullopt;
  const V3d anchor = samples.front();
  if (!anchor.allFinite()) return std::nullopt;
  double scale = 0.0;
  for (const V3d& point : samples) {
    const V3d delta = point - anchor;
    if (!point.allFinite() || !delta.allFinite()) return std::nullopt;
    scale = std::max(scale, delta.stableNorm());
  }
  if (!std::isfinite(scale) || scale <= GeomConst::Eps) return std::nullopt;
  QVector<V3d> local;
  local.reserve(samples.size());
  for (const V3d& point : samples) local.append((point - anchor) / scale);

  auto fit = local.size() == 3 ? circumcircle(local) : initialCircle(local);
  if (!fit) return std::nullopt;
  if (local.size() > 3) {
    fit = refine(local, *fit);
    if (!fit || !wellConstrained(local, *fit)) return std::nullopt;
  }
  int dominant = 0;
  for (int i = 1; i < 3; ++i)
    if (std::abs(fit->normal[i]) > std::abs(fit->normal[dominant])) dominant = i;
  if (fit->normal[dominant] < 0.0) fit->normal = -fit->normal;
  const V3d center = anchor + scale * fit->center;
  const double radius = scale * fit->radius;
  if (!center.allFinite() || !std::isfinite(radius) || radius <= GeomConst::Eps)
    return std::nullopt;
  Circle circle(std::move(points), center, fit->normal, radius);
  return std::isfinite(circle.rmsResidual()) ? std::optional<Circle>{std::move(circle)} : std::nullopt;
}

Circle::Circle(QVector<V3d> points, const V3d& center, const V3d& normal, double radius)
  : m_points(std::move(points)), m_center(center), m_normal(normal), m_radius(radius)
{
  const Fit fit{m_center, m_normal, m_radius};
  double errorNorm = 0.0;
  for (const V3d& point : std::as_const(m_points)) {
    const auto values = residual(point, fit);
    if (!values) {
      m_rmsResidual = std::numeric_limits<double>::infinity();
      return;
    }
    errorNorm = std::hypot(errorNorm, (*values)[0], (*values)[1]);
  }
  m_rmsResidual = errorNorm / std::sqrt(static_cast<double>(m_points.size()));
}
