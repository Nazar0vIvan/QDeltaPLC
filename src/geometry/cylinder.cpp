#include "cylinder.h"

namespace {

bool isValidInput(const V3d& axisDir, double radius)
{
  return axisDir.allFinite() &&
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

std::optional<M3d> basisFromAxis(const V3d& axisDir, Axis axis) {
  const auto unitAxis = normalize(axisDir);

  if (!unitAxis || !unitAxis->allFinite()) {
    return std::nullopt;
  }

  const V3d helper = leastParallelUnit(*unitAxis);
  const auto helperProjected = normalize(helper - helper.dot(*unitAxis) * *unitAxis);

  if (!helperProjected) return std::nullopt;

  V3d x; V3d y; V3d z;

  switch (axis) {
    case Axis::X: {
      x = *unitAxis;
      y = *helperProjected;
      z = x.cross(y);
      break;
    }
    case Axis::Y: {
      x = *helperProjected;
      y = *unitAxis;
      z = x.cross(y);
      break;
    }
    case Axis::Z: {
      x = *helperProjected;
      z = *unitAxis;
      y = z.cross(x);
      break;
    }
  }

  return basis2rot(x, y, z);
}

} // namespace


std::optional<Cylinder> Cylinder::fromAxis(const V3d &axisDir, double radius, Axis axis)
{
  if (!isValidInput(axisDir, radius)) {
    return std::nullopt;
  }
}
