#include "cylinder.h"

#include <cmath>

#include "utils.h"

namespace {

bool isValidInput(const V3d& axisDir, const V3d& origin, double radius)
{
  return axisDir.allFinite() &&
         origin.allFinite() &&
         std::isfinite(radius) &&
         radius > GeomConst::Eps;
}

V3d leastParallelUnit(const V3d& v)
{
  const double ax = std::abs(v.x());
  const double ay = std::abs(v.y());
  const double az = std::abs(v.z());

  if (ax <= ay && ax <= az) return V3d::UnitX();
  if (ay <= ax && ay <= az) return V3d::UnitY();

  return V3d::UnitZ();
}

Axis radialAxis(Axis axis)
{
  switch (axis) {
    case Axis::X: return Axis::Y;
    case Axis::Y: return Axis::Z;
    case Axis::Z: return Axis::Y;
  }

  return Axis::Y;
}

std::optional<V3d> projectedWorldAxis(const V3d& worldAxis, const V3d& normal, double eps = GeomConst::Eps)
{
  return normalize(worldAxis - worldAxis.dot(normal) * normal, eps);
}

std::optional<OrthoBasis> basisFromAxis(const V3d& axisDir, Axis axis, double eps = GeomConst::Eps)
{
  const auto u = normalize(axisDir, eps);

  if (!u) return std::nullopt;

  V3d e1, e2, e3;

  switch (axis) {
    case Axis::X: {
      e1 = *u;
      const auto y = projectedWorldAxis(V3d::UnitY(), e1, eps);
      if (!y) return std::nullopt;
      e2 = *y;
      e3 = e1.cross(e2);
      break;
    }
    case Axis::Y: {
      e2 = *u;
      const auto z = projectedWorldAxis(V3d::UnitZ(), e2, eps);
      if (!z) return std::nullopt;
      e3 = *z;
      e1 = e2.cross(e3);
      break;
    }
    case Axis::Z: {
      e3 = *u;
      const auto x = projectedWorldAxis(V3d::UnitX(), e3, eps);
      if (!x) return std::nullopt;
      e1 = *x;
      e2 = e3.cross(e1);
      break;
    }
  }

  return vecs2basis(e1, e2, e3, eps);
}

} // namespace

std::optional<Cylinder> Cylinder::fromAxis(const V3d& axisDir, const V3d& origin, double radius, Axis axis)
{
  if (!isValidInput(axisDir, origin, radius)) {
    return std::nullopt;
  }

  const auto basis = basisFromAxis(axisDir, axis);

  if (!basis) return std::nullopt;

  const auto originPose = Pose::fromAxes(basis->e1, basis->e2, basis->e3, origin);

  if (!originPose) return std::nullopt;

  Cylinder cylinder;
  cylinder.m_axisDir = *normalize(axisDir);
  cylinder.m_originPose = *originPose;
  cylinder.m_radius = radius;
  cylinder.m_axis = axis;

  return cylinder;
}

void Cylinder::setSurfacePose(double offset, double angleDeg)
{
  if (!std::isfinite(offset) || !std::isfinite(angleDeg)) {
    return;
  }

  const M4d localTf =
      makeTranslation(axisVec(m_axis, offset)) *
      makeRotation(-angleDeg, m_axis) *
      makeTranslation(axisVec(radialAxis(m_axis), m_radius));

  const auto pose = Pose::fromTransform(m_originPose.transform() * localTf);

  if (pose) m_surfacePose = *pose;
}

QVector<Pose> Cylinder::surfaceRing(int n, double offset) const
{
  QVector<Pose> poses;

  if (n <= 0 || !std::isfinite(offset)) {
    return poses;
  }

  poses.reserve(n);

  for (int i = 0; i < n; ++i) {
    const double angleDeg =
        360.0 * static_cast<double>(i) / static_cast<double>(n);

    const M4d localTf =
        makeTranslation(axisVec(m_axis, offset)) *
        makeRotation(angleDeg, m_axis) *
        makeTranslation(axisVec(radialAxis(m_axis), -m_radius));

    const auto pose = Pose::fromTransform(m_originPose.transform() * localTf);

    if (pose) {
      poses.push_back(*pose);
    }
  }

  return poses;
}

V3d Cylinder::axisDir() const
{
  return m_axisDir;
}

Pose Cylinder::originPose() const
{
  return m_originPose;
}

double Cylinder::radius() const
{
  return m_radius;
}

Axis Cylinder::axis() const
{
  return m_axis;
}

Pose Cylinder::surfacePose() const
{
  return m_surfacePose;
}
