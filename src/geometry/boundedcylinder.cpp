#include "boundedcylinder.h"

#include "utils.h"

#include <Eigen/Cholesky>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

using Point = BoundedCylinder::Point;

struct Fit {
  V3d axis;
  V3d center; // Relative to the sample centroid, perpendicular to the axis.
  double radius = 0.0;
  double cost = std::numeric_limits<double>::infinity();
};

Point coordinates(const V3d& value)
{
  return {value.x(), value.y(), value.z()};
}

bool tangentBasis(const V3d& axis, V3d& u, V3d& v)
{
  const V3d reference = std::abs(axis.x()) < 0.9 ? V3d{1, 0, 0} : V3d{0, 1, 0};
  const auto tangent = normalize(reference - reference.dot(axis) * axis);
  if (!tangent) return false;
  u = *tangent;
  v = axis.cross(u);
  return v.allFinite();
}

bool residuals(const std::vector<V3d>& points, const Fit& fit, Eigen::VectorXd& result)
{
  if (!fit.axis.allFinite() || !fit.center.allFinite() || !std::isfinite(fit.radius)
      || fit.radius <= GeomConst::Eps) return false;
  result.resize(static_cast<Eigen::Index>(points.size()));
  for (std::size_t i = 0; i < points.size(); ++i) {
    const V3d delta = points[i] - fit.center;
    const double radial = (delta - delta.dot(fit.axis) * fit.axis).norm();
    if (!std::isfinite(radial) || radial <= GeomConst::Eps) return false;
    result[static_cast<Eigen::Index>(i)] = radial - fit.radius;
  }
  return result.allFinite();
}

std::optional<Fit> displaced(const Fit& fit, const V3d& u, const V3d& v,
                             const Eigen::Matrix<double, 5, 1>& delta)
{
  const auto axis = normalize(fit.axis + delta[0] * u + delta[1] * v);
  if (!axis) return std::nullopt;
  const V3d shifted = fit.center + delta[2] * u + delta[3] * v;
  Fit next;
  next.axis = *axis;
  next.center = shifted - shifted.dot(next.axis) * next.axis;
  next.radius = fit.radius + delta[4];
  if (!next.center.allFinite() || !std::isfinite(next.radius)
      || next.radius <= GeomConst::Eps) return std::nullopt;
  return next;
}

std::optional<Fit> fitFromAxis(const std::vector<V3d>& points, const V3d& initialAxis,
                               double scale)
{
  const auto axis = normalize(initialAxis);
  if (!axis) return std::nullopt;
  Fit fit;
  fit.axis = *axis;
  for (const V3d& point : points) fit.radius += (point - point.dot(fit.axis) * fit.axis).norm();
  fit.radius /= static_cast<double>(points.size());
  if (!std::isfinite(fit.radius) || fit.radius <= GeomConst::Eps) return std::nullopt;

  Eigen::VectorXd values;
  if (!residuals(points, fit, values)) return std::nullopt;
  fit.cost = values.squaredNorm();
  double damping = 1e-3;
  for (int iteration = 0; iteration < 200; ++iteration) {
    V3d u, v;
    if (!tangentBasis(fit.axis, u, v)) return std::nullopt;
    Eigen::MatrixXd jacobian(values.size(), 5);
    for (int column = 0; column < 5; ++column) {
      Eigen::Matrix<double, 5, 1> step = Eigen::Matrix<double, 5, 1>::Zero();
      step[column] = column < 2 ? 1e-5 : std::max(1e-6, scale * 1e-5);
      const auto perturbed = displaced(fit, u, v, step);
      Eigen::VectorXd sample;
      if (!perturbed || !residuals(points, *perturbed, sample)) return std::nullopt;
      jacobian.col(column) = (sample - values) / step[column];
    }

    const Eigen::Matrix<double, 5, 1> gradient = jacobian.transpose() * values;
    const Eigen::Matrix<double, 5, 5> normal = jacobian.transpose() * jacobian;
    if (!gradient.allFinite() || !normal.allFinite()) return std::nullopt;
    bool improved = false;
    for (int trial = 0; trial < 12; ++trial) {
      Eigen::Matrix<double, 5, 5> regularized = normal;
      for (int i = 0; i < 5; ++i) regularized(i, i) += damping * std::max(1.0, normal(i, i));
      const Eigen::Matrix<double, 5, 1> change = regularized.ldlt().solve(-gradient);
      if (!change.allFinite()) return std::nullopt;
      const auto candidate = displaced(fit, u, v, change);
      Eigen::VectorXd candidateValues;
      if (candidate && residuals(points, *candidate, candidateValues)) {
        const double cost = candidateValues.squaredNorm();
        if (std::isfinite(cost) && cost < fit.cost) {
          const double previous = fit.cost;
          fit = *candidate;
          fit.cost = cost;
          values = std::move(candidateValues);
          damping = std::max(1e-12, damping / 3.0);
          improved = true;
          if (previous - cost <= 1e-12 * std::max(1.0, previous)) return fit;
          break;
        }
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

bool wellConstrained(const std::vector<V3d>& points, const Fit& fit, double scale)
{
  // The five fitted parameters must each affect the radial residuals. This
  // rejects single rings, narrow arcs, and other ambiguous point layouts.
  V3d u, v;
  if (!tangentBasis(fit.axis, u, v)) return false;
  Eigen::VectorXd values;
  if (!residuals(points, fit, values)) return false;
  Eigen::MatrixXd jacobian(values.size(), 5);
  for (int column = 0; column < 5; ++column) {
    Eigen::Matrix<double, 5, 1> step = Eigen::Matrix<double, 5, 1>::Zero();
    step[column] = column < 2 ? 1e-5 : std::max(1e-6, scale * 1e-5);
    const auto perturbed = displaced(fit, u, v, step);
    Eigen::VectorXd sample;
    if (!perturbed || !residuals(points, *perturbed, sample)) return false;
    jacobian.col(column) = (sample - values) / step[column];
  }
  jacobian.col(0) /= scale;
  jacobian.col(1) /= scale;
  const Eigen::JacobiSVD<Eigen::MatrixXd> svd(jacobian);
  const auto singular = svd.singularValues();
  return singular.size() == 5 && singular[0] > 0.0
      && singular[4] / singular[0] >= 0.01;
}

} // namespace

std::optional<BoundedCylinder> BoundedCylinder::fromPoints(const std::vector<Point>& points)
{
  if (points.size() < 6) return std::nullopt;
  V3d centroid = V3d::Zero();
  for (const Point& point : points) {
    const V3d value(point[0], point[1], point[2]);
    if (!value.allFinite()) return std::nullopt;
    centroid += value;
  }
  centroid /= static_cast<double>(points.size());
  if (!centroid.allFinite()) return std::nullopt;

  std::vector<V3d> centered;
  centered.reserve(points.size());
  M3d covariance = M3d::Zero();
  for (const Point& point : points) {
    const V3d value(point[0], point[1], point[2]);
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
    const auto candidate = fitFromAxis(centered, initial, scale);
    if (candidate && (!best || candidate->cost < best->cost)) best = candidate;
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
  BoundedCylinder result;
  result.points = points;
  result.origin = coordinates(origin);
  result.axis = coordinates(axis);
  result.radius = best->radius;
  result.length = length;
  result.rmsResidual = rms;
  return result;
}

std::optional<BoundedCylinder> BoundedCylinder::fromJsonFile(const QString& path)
{
  const auto samples = readProbeSamples(path);
  if (!samples) return std::nullopt;
  std::vector<Point> points;
  points.reserve(samples->points.size());
  for (const V3d& sample : samples->points) points.push_back(coordinates(sample));
  auto cylinder = fromPoints(points);
  if (!cylinder) return std::nullopt;
  cylinder->radius += samples->dir * samples->radius;
  if (!std::isfinite(cylinder->radius) || cylinder->radius <= GeomConst::Eps)
    return std::nullopt;
  return cylinder;
}
