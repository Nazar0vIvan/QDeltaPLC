#include "planemesh.h"
#include "geometry/utils.h"

#include <cmath>
#include <stdexcept>

namespace {

bool isValidMeshAxis(const PlaneMeshAxis& axis)
{
  if (axis.count <= 0) {
    return false;
  }

  if (!std::isfinite(axis.step)) {
    return false;
  }

  if (!axis.dir.allFinite()) {
    return false;
  }

  if (axis.dir.squaredNorm() <= GeomConst::Eps * GeomConst::Eps) {
    return false;
  }

  if (axis.count > 1 && std::abs(axis.step) <= GeomConst::Eps) {
    return false;
  }

  return true;
}

double planeValue(const Plane& plane, const V3d& point)
{
  // CHANGE: Plane no longer has A/B/C/D fields; use coeffs
  return plane.coeffs.dot(V4d{point.x(), point.y(), point.z(), 1.0});
}

std::optional<OrthoBasis> makePlaneBasis(const PlanePoseMeshParams& params)
{
  const auto x = normalize(params.axis1.dir);
  const auto y = normalize(params.axis2.dir);
  const auto z = normalize(params.plane.normal());

  if (!x || !y || !z) return std::nullopt;

  return vecs2basis(*x, *y, *z);
}

} // namespace

bool PoseMesh::empty() const noexcept
{
  return poses.isEmpty();
}

int PoseMesh::rows() const noexcept
{
  return poses.size();
}

int PoseMesh::cols() const noexcept
{
  return poses.isEmpty() ? 0 : poses.front().size();
}

int PoseMesh::size() const noexcept
{
  // CHANGE: robust for accidentally non-rectangular public poses
  int total = 0;

  for (const auto& row : poses) {
    total += row.size();
  }

  return total;
}

Pose& PoseMesh::at(int row, int col)
{
  // CHANGE: check against concrete row size, not cols(),
  // because public poses can theoretically be non-rectangular
  if (row < 0 || row >= rows() || col < 0 || col >= poses[row].size()) {
    throw std::out_of_range("PoseMesh index is out of range");
  }

  return poses[row][col];
}

const Pose& PoseMesh::at(int row, int col) const
{
  // CHANGE: check against concrete row size, not cols()
  if (row < 0 || row >= rows() || col < 0 || col >= poses[row].size()) {
    throw std::out_of_range("PoseMesh index is out of range");
  }

  return poses[row][col];
}

std::optional<PoseMesh> makePlanePoseMesh(const PlanePoseMeshParams& params)
{
  if (!params.origin.allFinite()) {
    return std::nullopt;
  }

  if (!params.plane.coeffs.allFinite()) {
    return std::nullopt;
  }

  if (!isValidMeshAxis(params.axis1) || !isValidMeshAxis(params.axis2)) {
    return std::nullopt;
  }

  const auto basis = makePlaneBasis(params);

  if (!basis) return std::nullopt;

  V3d origin = params.origin;

  PoseMesh mesh;
  mesh.poses.resize(params.axis1.count);

  for (int row = 0; row < params.axis1.count; ++row) {
    mesh.poses[row].reserve(params.axis2.count);

    for (int col = 0; col < params.axis2.count; ++col) {
      const V3d point =
          origin +
          static_cast<double>(row) * params.axis1.step * basis->e1 +
          static_cast<double>(col) * params.axis2.step * basis->e2;

      const auto pose = Pose::fromAxes(basis->e1, basis->e2, basis->e3, point);

      if (!pose) return std::nullopt;

      mesh.poses[row].push_back(*pose);
    }
  }

  return mesh;
}



