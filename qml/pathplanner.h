#ifndef PATHPLANNER_H
#define PATHPLANNER_H

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

// ------------ Math ------------

using Vec6d = Eigen::Matrix<double, 6, 1>; // row 6x1

struct Plane {
  double A, B, C, D;   // A*x + B*y + C*z + D = 0
  double AA, BB, DD;   // z = AA*x + BB*y + DD
};

struct Pose {
  Pose() = default;
  Pose(const Vec6d& frame_, const Eigen::Matrix4d& T_);

  static Pose fromTransform(const Eigen::Matrix4d& T_);
  static Pose fromAxes(const Eigen::Vector3d& t_,
                       const Eigen::Vector3d& b_,
                       const Eigen::Vector3d& n_,
                       const Eigen::Vector3d& p_);

  Vec6d frame;
  Eigen::Matrix4d T;

  Eigen::Vector3d t{Eigen::Vector3d::Zero()};
  Eigen::Vector3d b{Eigen::Vector3d::Zero()};
  Eigen::Vector3d n{Eigen::Vector3d::Zero()};
  Eigen::Vector3d p{Eigen::Vector3d::Zero()};

  Eigen::Vector3d pos() const { return T.block<3,1>(0,3); }
  Eigen::Matrix3d rot() const { return T.block<3,3>(0,0); }
};

struct Cylinder {
  static Cylinder fromTwoPoints(const Eigen::Vector3d& c1,
                                const Eigen::Vector3d& c2,
                                const Eigen::Vector3d& pc,
                                double R, double L,
                                char axis = 'y');

  static Cylinder fromAxis(const Eigen::Vector3d& u,
                           const Eigen::Vector3d& pc,
                           double R, double L,
                           char axis = 'y');

  Pose surfacePose(char axis1, double val1,
                   char axisRot, double angleDeg,
                   char axis2, double val2,
                   bool returnLocal = false) const;

  QVector<Pose> surfaceRing(int n, double L) const;

  double R;
  double L;
  Pose pose;
};

struct EulerSolution {
  double A1, A2,
         B1, B2,
         C1, C2;
};

Eigen::Matrix4d trMatrix4x4(const Eigen::Vector3d& delta);
Eigen::Matrix4d rotMatrix4x4(double angleDeg, char axis);
Plane pointsToPlane(const Eigen::Ref<const Eigen::VectorXd>& x,
                    const Eigen::Ref<const Eigen::VectorXd>& y,
                    const Eigen::Ref<const Eigen::VectorXd>& z);
Eigen::Vector3d poly(double x0, double x1, double x2,
                     double y0, double y1, double y2);

EulerSolution rot2euler(const Eigen::Matrix3d& R, bool is_deg = false);
Eigen::Matrix3d euler2rot(double A, double B, double C, bool is_deg = false);
Eigen::Vector3d axisVec(char axis, double value);
Eigen::Vector3d prjPointToLine(const Eigen::Vector3d& l0,
                               const Eigen::Vector3d& v,
                               const Eigen::Vector3d& p);

// project vector vec onto plane ⟂ n (n assumed unit-ish)
Eigen::Vector3d prjToPerpPlane(const Eigen::Vector3d& vec, const Eigen::Vector3d& n);

// ------------ Frene ------------
Pose getFreneByPoly(const Eigen::Vector3d& p0,
                    const Eigen::Vector3d& u1,
                    const Eigen::Vector3d& u2,
                    const Eigen::Vector3d& v1);

Pose getFreneByCirc(const Eigen::Vector3d& pt0,
                    const Eigen::Vector3d& ptc);

// ------------ Blade ------------
using Profile = QVector<Eigen::Vector3d>;
struct BladeProfile {
  Profile cx, cv, re, le;
};
using Airfoil = QVector<BladeProfile>;

Vec6d getBeltFrame(const Eigen::Vector3d& o,
                   const Eigen::Ref<const Eigen::VectorXd>& x,
                   const Eigen::Ref<const Eigen::VectorXd>& y,
                   const Eigen::Ref<const Eigen::VectorXd>& z);

Eigen::Vector3d jsonValueToVec3(const QJsonValue& v);
Profile jsonArrayToProfile(const QJsonArray& arr);
BladeProfile jsonObjectToBladeProfile(const QJsonObject& obj);
Airfoil loadBladeJson(const QString& filePath);

Pose cxProfileStartFrene(const Profile& cx,
                         const Profile& cx_next,
                         double L);

Pose cxProfileEndFrene(const Profile& cx,
                       const Profile& cx_next,
                       double L);

QVector<Pose> cxProfileFrenes(const Profile& cx,
                              const Profile& cx_next,
                              double L);

// ------------ Rsi Trajectory ------------
struct MotionParams {
  double v, a
};

namespace rsi {
  QVector<Vec6d> polyline(const QVector<Vec6d> &ref_points, const MotionParams& mp, int decimals = 3);
  QVector<Vec6d> lin(const Vec6d& P1, const Vec6d& P2, const MotionParams& mp, int decimals = 3);
};

QVector<Pose> pathFromSurfPoses(const QVector<Pose>& surf_poses, const Eigen::Matrix4d& AiT);

QVector<Vec6d> posesToFrames(const QVector<Pose>& poses);

void writeOffsetsToJson(const QVector<Vec6d>& offsets, const QString& filePath, int decimals = 3);
#endif // PATHPLANNER_H
