#include "boundedcylinder.h"

#include "utils.h"

#include <Eigen/Cholesky>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace {

using Point = BoundedCylinder::Point;

struct Fit {
  V3d axis = V3d::Zero();
  V3d center = V3d::Zero(); // Relative to the sample centroid, perpendicular to the axis.
  double radius = 0.0;
  double cost = std::numeric_limits<double>::infinity();
  Eigen::VectorXd values; // Radial residuals for these fitted parameters.
};

Point coordinates(const V3d& value)
{
  return {value.x(), value.y(), value.z()};
}

std::optional<std::array<V3d, 2>> tangentBasis(const V3d& axis)
{
  const V3d reference = std::abs(axis.x()) < 0.9 ? V3d{1, 0, 0} : V3d{0, 1, 0};
  const auto tangent = normalize(reference - reference.dot(axis) * axis);
  if (!tangent) return std::nullopt;
  const V3d bitangent = axis.cross(*tangent);
  if (!bitangent.allFinite()) return std::nullopt;
  return std::array<V3d, 2>{*tangent, bitangent};
}

std::optional<double> residual(const V3d& point, const Fit& fit)
{
  const V3d delta = point - fit.center;
  const double radial = (delta - delta.dot(fit.axis) * fit.axis).norm();
  if (!std::isfinite(radial) || radial <= GeomConst::Eps) return std::nullopt;
  const double value = radial - fit.radius;
  return std::isfinite(value) ? std::optional<double>{value} : std::nullopt;
}

std::optional<Fit> evaluate(const std::vector<V3d>& points, Fit fit)
{
  if (!fit.axis.allFinite() || !fit.center.allFinite() || !std::isfinite(fit.radius)
      || fit.radius <= GeomConst::Eps) return std::nullopt;
  fit.values.resize(static_cast<Eigen::Index>(points.size()));
  for (std::size_t i = 0; i < points.size(); ++i) {
    const auto value = residual(points[i], fit);
    if (!value) return std::nullopt;
    fit.values[static_cast<Eigen::Index>(i)] = *value;
  }
  fit.cost = fit.values.squaredNorm();
  return fit;
}

// A fit, its tangent frame and an increment define one numerical displacement.
std::optional<Fit> displaced(const Fit& fit, const std::array<V3d, 2>& basis, const Eigen::Matrix<double, 5, 1>& delta)
{
  const auto axis = normalize(fit.axis + delta[0] * basis[0] + delta[1] * basis[1]);
  if (!axis) return std::nullopt;
  const V3d shifted = fit.center + delta[2] * basis[0] + delta[3] * basis[1];
  Fit next;
  next.axis = *axis;
  next.center = shifted - shifted.dot(next.axis) * next.axis;
  next.radius = fit.radius + delta[4];
  if (!next.center.allFinite() || !std::isfinite(next.radius)
      || next.radius <= GeomConst::Eps) return std::nullopt;
  return next;
}

// Samples and an evaluated fit define the derivatives; scale sets the finite
// difference steps, and the owned matrix is returned for reuse across iterations.
std::optional<Eigen::MatrixXd> linearize(const std::vector<V3d>& points, const Fit& fit, double scale, Eigen::MatrixXd jacobian)
{
  const auto basis = tangentBasis(fit.axis);
  if (!basis) return std::nullopt;
  jacobian.resize(fit.values.size(), 5);
  for (int column = 0; column < 5; ++column) {
    Eigen::Matrix<double, 5, 1> step = Eigen::Matrix<double, 5, 1>::Zero();
    step[column] = column < 2 ? 1e-5 : std::max(1e-6, scale * 1e-5);
    const auto perturbed = displaced(fit, *basis, step);
    if (!perturbed) return std::nullopt;
    for (std::size_t i = 0; i < points.size(); ++i) {
      const auto sample = residual(points[i], *perturbed);
      if (!sample) return std::nullopt;
      const auto row = static_cast<Eigen::Index>(i);
      jacobian(row, column) = *sample;
    }
    jacobian.col(column) = (jacobian.col(column) - fit.values) / step[column];
  }
  return jacobian;
}

// Samples, initial axis and sample scale define one fit initialization.
std::optional<Fit> fitFromAxis(const std::vector<V3d>& points, const V3d& initialAxis, double scale)
{
  const auto axis = normalize(initialAxis);
  if (!axis) return std::nullopt;
  Fit fit;
  fit.axis = *axis;
  for (const V3d& point : points) fit.radius += (point - point.dot(fit.axis) * fit.axis).norm();
  fit.radius /= static_cast<double>(points.size());
  if (!std::isfinite(fit.radius) || fit.radius <= GeomConst::Eps) return std::nullopt;

  auto evaluated = evaluate(points, std::move(fit));
  if (!evaluated) return std::nullopt;
  fit = std::move(*evaluated);
  Eigen::MatrixXd jacobian;
  Eigen::VectorXd trialValues;
  double damping = 1e-3;
  for (int iteration = 0; iteration < 200; ++iteration) {
    const auto basis = tangentBasis(fit.axis);
    if (!basis) return std::nullopt;
    auto derivatives = linearize(points, fit, scale, std::move(jacobian));
    if (!derivatives) return std::nullopt;
    jacobian = std::move(*derivatives);

    const Eigen::Matrix<double, 5, 1> gradient = jacobian.transpose() * fit.values;
    const Eigen::Matrix<double, 5, 5> normal = jacobian.transpose() * jacobian;
    if (!gradient.allFinite() || !normal.allFinite()) return std::nullopt;
    bool improved = false;
    for (int trial = 0; trial < 12; ++trial) {
      Eigen::Matrix<double, 5, 5> regularized = normal;
      for (int i = 0; i < 5; ++i) regularized(i, i) += damping * std::max(1.0, normal(i, i));
      const Eigen::Matrix<double, 5, 1> change = regularized.ldlt().solve(-gradient);
      if (!change.allFinite()) return std::nullopt;
      auto candidate = displaced(fit, *basis, change);
      if (candidate) {
        candidate->values = std::move(trialValues);
        candidate = evaluate(points, std::move(*candidate));
      }
      if (candidate) {
        const double cost = candidate->cost;
        if (std::isfinite(cost) && cost < fit.cost) {
          const double previous = fit.cost;
          trialValues = std::move(fit.values);
          fit = std::move(*candidate);
          damping = std::max(1e-12, damping / 3.0);
          improved = true;
          if (previous - cost <= 1e-12 * std::max(1.0, previous)) return fit;
          break;
        }
        trialValues = std::move(candidate->values);
      }
      damping *= 10.0;
    }
    if (!improved) {
      if (gradient.norm() <= 1e-7 * std::max(1.0, scale)) return fit;
      return std::nullopt;
    }
  }
  return std::nullopt;
}

// The observability check uses the samples, fitted parameters and their scale.
bool wellConstrained(const std::vector<V3d>& points, const Fit& fit, double scale)
{
  // The five fitted parameters must each affect the radial residuals. This
  // rejects single rings, narrow arcs, and other ambiguous point layouts.
  auto derivatives = linearize(points, fit, scale, {});
  if (!derivatives) return false;
  Eigen::MatrixXd jacobian = std::move(*derivatives);
  jacobian.col(0) /= scale;
  jacobian.col(1) /= scale;
  const Eigen::JacobiSVD<Eigen::MatrixXd> svd(jacobian);
  const auto singular = svd.singularValues();
  return singular.size() == 5 && singular[0] > 0.0
      && singular[4] / singular[0] >= 0.01;
}

} // namespace

std::optional<BoundedCylinder> BoundedCylinder::fromPoints(QVector<V3d> points)
{
  const auto& samples = std::as_const(points);
  if (samples.size() < 6) return std::nullopt;
  V3d centroid = V3d::Zero();
  for (const V3d& value : samples) {
    if (!value.allFinite()) return std::nullopt;
    centroid += value;
  }
  centroid /= static_cast<double>(points.size());
  if (!centroid.allFinite()) return std::nullopt;

  std::vector<V3d> centered;
  centered.reserve(points.size());
  M3d covariance = M3d::Zero();
  for (const V3d& value : samples) {
    centered.push_back(value - centroid);
    covariance += centered.back() * centered.back().transpose();
  }
  covariance /= static_cast<double>(points.size());
  Eigen::SelfAdjointEigenSolver<M3d> eigen(covariance);
  if (eigen.info() != Eigen::Success || !eigen.eigenvalues().allFinite()) return std::nullopt;
  const double scale = std::sqrt(std::max(0.0, eigen.eigenvalues()[2]));
  if (!std::isfinite(scale) || scale <= GeomConst::Eps) return std::nullopt;

  std::optional<Fit> best;
  for (int i = 0; i < 6; ++i) {
    V3d initial;
    if (i < 3) initial = eigen.eigenvectors().col(i);
    else initial = V3d::Unit(i - 3);
    auto candidate = fitFromAxis(centered, initial, scale);
    if (candidate && (!best || candidate->cost < best->cost)) best = std::move(candidate);
  }
  if (!best || !wellConstrained(centered, *best, scale)) return std::nullopt;

  V3d axis = best->axis;
  Eigen::Index dominant = 0;
  axis.cwiseAbs().maxCoeff(&dominant);
  if (axis[dominant] < 0.0) axis = -axis;
  double minimum = std::numeric_limits<double>::infinity();
  double maximum = -minimum;
  for (const V3d& point : centered) {
    const double projection = point.dot(axis);
    minimum = std::min(minimum, projection);
    maximum = std::max(maximum, projection);
  }
  const double length = maximum - minimum;
  const double rms = std::sqrt(best->cost / static_cast<double>(points.size()));
  const double tolerance = std::max(0.1, 0.02 * best->radius); // mm
  if (!std::isfinite(length) || length <= GeomConst::Eps
	 || !std::isfinite(best->radius) || best->radius <= GeomConst::Eps
	 || !std::isfinite(rms) || rms > tolerance) return std::nullopt;

  const V3d origin = centroid + best->center + (minimum + length / 2.0) * axis;
  if (!origin.allFinite()) return std::nullopt;
  return BoundedCylinder(std::move(points), {coordinates(origin), coordinates(axis)}, {best->radius, length}, rms);
}

std::optional<BoundedCylinder> BoundedCylinder::fromJsonFile(const QString& path)
{
  auto samples = readProbeSamples(path);
  return samples ? fromSamples(std::move(*samples)) : std::nullopt;
}

std::optional<BoundedCylinder> BoundedCylinder::fromSamples(ProbeSamples samples)
{
	if (!std::isfinite(samples.radius) || samples.radius < 0.0 || (samples.dir != 1 && samples.dir != -1)) return std::nullopt;
  auto cylinder = fromPoints(std::move(samples.points));
  if (!cylinder) return std::nullopt;
  if (!cylinder->applyProbe(samples.radius, samples.dir)) return std::nullopt;
  return cylinder;
}

BoundedCylinder::BoundedCylinder(QVector<V3d> points, const std::array<Point, 2>& frame, const std::array<double, 2>& dimensions, double residual)
  : m_points(std::move(points)), m_origin(frame[0]), m_axis(frame[1]), m_radius(dimensions[0]),
    m_length(dimensions[1]), m_rmsResidual(residual)
{}

bool BoundedCylinder::applyProbe(double radius, int dir)
{
  m_radius += dir * radius;
  return std::isfinite(m_radius) && m_radius > GeomConst::Eps;
}
