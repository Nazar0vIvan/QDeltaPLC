#include "cylinder.h"

Cylinder Cylinder::fromAxis(const V3d &u, const V3d &pc, double R, double L, char axis)
{
  V3d y = u.normalized();

         // choose helper not parallel to y
  const V3d helper =
      (std::abs(y.x()) < 0.9) ? V3d::UnitX() : V3d::UnitY();

  V3d x = helper - helper.dot(y) * y;
  x.normalize();

  V3d z = x.cross(y).normalized();
  M4d transform = M4d::Identity();
  transform.block<3,3>(0,0) << x, y, z;
  transform.block<3,1>(0,3) = pc;

         // EulerSolution angles = rot2euler(transform.topLeftCorner<3,3>(), true);

         // V6d frame;
         // frame << pc.x(), pc.y(), pc.z(), angles.A1, angles.B1, angles.C1;

  Pose pose = Pose::fromTransform(transform);
  return { R, L, pose };
}

Cylinder Cylinder::fromTwoPoints(const V3d& c1, const V3d& c2,
                         const V3d& o, double R,
                         double L, char axis)
{
  axis = char(std::tolower(static_cast<unsigned char>(axis)));
  V3d d = (c2 - c1);
  const double dn = d.norm();
  d /= dn;

  V3d x, y, z;
  if (axis == 'y') {
      y = d;
      z = prjToPerpPlane(V3d::UnitZ(), y);
      x = y.cross(z).normalized();
    } else if (axis == 'z') {
      z = d;
      x = prjToPerpPlane(V3d::UnitX(), z);
      y = z.cross(x).normalized();
    }

  M4d transform = M4d::Identity();
  transform.block<3,3>(0,0) << x, y, z;
  transform.block<3,1>(0,3) = o;

         // EulerSolution angles = rot2euler(transform.topLeftCorner<3,3>(), true);

         // V6d frame;
         // frame << o.x(), o.y(), o.z(), angles.A1, angles.B1, angles.C1;

         // Pose pose = { frame, transform };
  return { R, L, Pose::fromTransform(transform) };
}

Pose Cylinder::surfacePose(char axis1, double val1,
                           char axisRot, double angleDeg,
                           char axis2, double val2,
                           bool returnLocal) const
{
  const M4d T1 = trMatrix4x4(axisVec(axis1, val1));
  const M4d RR = rotMatrix4x4(angleDeg, axisRot);
  const M4d T2 = trMatrix4x4(axisVec(axis2, val2));

  const M4d T_local = T1 * RR * T2;

  Pose surf_pose;
  surf_pose.transf = returnLocal ? T_local : pose.transf * T_local;

  const V3d os = surf_pose.transf.block<3,1>(0,3);
  EulerSolution eul = rot2euler(surf_pose.transf.topLeftCorner<3,3>(), true);

  surf_pose.frame << os.x(), os.y(), os.z(), eul.A1, eul.B1, eul.C1;
  return surf_pose;
}

QVector<Pose> Cylinder::surfaceRing(int n, double L) const
{
  QVector<Pose> poses;
  poses.reserve(n);

  for (int k = 0; k < n; ++k) {
    const double angleDeg = 360.0 * double(k) / double(n);
    Pose p = surfacePose('z', L, 'z', angleDeg, 'y', -R, /*returnLocal=*/true);
    poses.push_back(p);
    }
  return poses;
}