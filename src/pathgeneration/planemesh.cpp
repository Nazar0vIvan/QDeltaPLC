#include "planemesh.h"

constexpr double kPlaneMeshEps = 1e-9;
constexpr double kPlaneMeshBasisTol = 1e-6;

V3d unitOrThrow(const V3d& v, const char* message)
{
  const double norm = v.norm();

  if (norm <= kPlaneMeshEps)
    throw std::invalid_argument(message);

  return v / norm;
}

V3d normalOfPlane(const Plane& plane)
{
  return unitOrThrow(V3d(plane.A, plane.B, plane.C),
                      "Plane normal is degenerate");
}

double planeValue(const Plane& plane, const V3d& p)
{
  return plane.A * p.x() +
         plane.B * p.y() +
         plane.C * p.z() +
         plane.D;
}

V3d projectPointToPlane(const Plane& plane, const V3d& p)
{
  const V3d n(plane.A, plane.B, plane.C);
  const double nn = n.squaredNorm();

  if (nn <= kPlaneMeshEps * kPlaneMeshEps)
    throw std::invalid_argument("Plane normal is degenerate");

  return p - planeValue(plane, p) / nn * n;
}

void validateMeshAxis(const PlaneMeshAxis& axis, const char* name)
{
  if (axis.count <= 0)
    throw std::invalid_argument(std::string(name) + ".count must be positive");

  if (axis.dir.norm() <= kPlaneMeshEps)
    throw std::invalid_argument(std::string(name) + ".dir is degenerate");

  if (axis.count > 1 && std::abs(axis.step) <= kPlaneMeshEps)
    throw std::invalid_argument(std::string(name) + ".step must be non-zero when count > 1");
}

void validatePlaneBasis(const V3d& x, const V3d& y, const V3d& z)
{
  if (std::abs(x.dot(z)) > kPlaneMeshBasisTol)
    throw std::invalid_argument("axis1.dir must belong to the plane");

  if (std::abs(y.dot(z)) > kPlaneMeshBasisTol)
    throw std::invalid_argument("axis2.dir must belong to the plane");

  if (std::abs(x.dot(y)) > kPlaneMeshBasisTol)
    throw std::invalid_argument("axis1.dir and axis2.dir must be orthogonal");

  if ((x.cross(y) - z).norm() > kPlaneMeshBasisTol)
    throw std::invalid_argument(
        "axis1.dir, axis2.dir and plane normal must form a right-handed basis");
}

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
  return rows() * cols();
}

Pose& PoseMesh::at(int row, int col)
{
  if (row < 0 || row >= rows() || col < 0 || col >= cols())
    throw std::out_of_range("PoseMesh index is out of range");

  return poses[row][col];
}

const Pose& PoseMesh::at(int row, int col) const
{
  if (row < 0 || row >= rows() || col < 0 || col >= cols())
    throw std::out_of_range("PoseMesh index is out of range");

  return poses[row][col];
}

PoseMesh makePlanePoseMesh(const PlanePoseMeshParams& params)
{
  validateMeshAxis(params.axis1, "axis1");
  validateMeshAxis(params.axis2, "axis2");

  const V3d x = unitOrThrow(params.axis1.dir, "axis1.dir is degenerate");
  const V3d y = unitOrThrow(params.axis2.dir, "axis2.dir is degenerate");
  const V3d z = normalOfPlane(params.plane);

  validatePlaneBasis(x, y, z);

  const V3d origin = params.projectOriginToPlane
                         ? projectPointToPlane(params.plane, params.origin)
                         : params.origin;

  PoseMesh mesh;
  mesh.poses.resize(params.axis1.count);

  for (int row = 0; row < params.axis1.count; ++row) {
      mesh.poses[row].reserve(params.axis2.count);

      for (int col = 0; col < params.axis2.count; ++col) {
          const V3d p =
              origin +
              static_cast<double>(row) * params.axis1.step * x +
              static_cast<double>(col) * params.axis2.step * y;

          mesh.poses[row].push_back(Pose::fromAxes(x, y, z, p));
        }
    }

  return mesh;
}
