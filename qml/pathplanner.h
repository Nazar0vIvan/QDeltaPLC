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

using V6d = Eigen::Matrix<double, 6, 1>; // row 6x1
using V3d = Eigen::Vector3d;
using M3d = Eigen::Matrix3d;
using M4d = Eigen::Matrix4d;

struct LoadBladeResult {

};

struct Plane {
  double A, B, C, D;   // A*x + B*y + C*z + D = 0
  double AA, BB, DD;   // z = AA*x + BB*y + DD
};

struct Pose {
  Pose() = default;
  static Pose fromFrame(const V6d& frame_);
  static Pose fromTransform(const M4d& T_);
  static Pose fromAxes(const V3d& t_, const V3d& b_, const V3d& n_, const V3d& p_);

  V6d frame{V6d::Zero()};
  M4d transf{M4d::Identity()};

  V3d t{V3d::Zero()};
  V3d b{V3d::Zero()};
  V3d n{V3d::Zero()};
  V3d p{V3d::Zero()};

  V3d pos() const { return transf.block<3,1>(0,3); }
  M3d rot() const { return transf.block<3,3>(0,0); }

private:
  void syncTransfByAxes();
  void syncAxesByTransf();
  void syncFrameByTransf();
  void syncTransfByFrame();
};

struct Cylinder {
  static Cylinder fromTwoPoints(const V3d& c1,
                                const V3d& c2,
                                const V3d& pc,
                                double R, double L,
                                char axis = 'y');

  static Cylinder fromAxis(const V3d& u,
                           const V3d& pc,
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

M4d trMatrix4x4(const V3d& delta);
M4d rotMatrix4x4(double angleDeg, char axis);
Plane pointsToPlane(const Eigen::Ref<const Eigen::VectorXd>& x,
                    const Eigen::Ref<const Eigen::VectorXd>& y,
                    const Eigen::Ref<const Eigen::VectorXd>& z);

V3d poly(const V3d &p0, const V3d &p1, const V3d &p2);

EulerSolution rot2euler(const M3d& R, bool is_deg = false);
M3d euler2rot(double A, double B, double C, bool is_deg = false);
V3d axisVec(char axis, double value);
V3d prjPointToLine(const V3d& l0, const V3d& v, const V3d& p);
V3d prjToPerpPlane(const V3d& vec, const V3d& n);

V3d tanByPoly(const V3d& p, const V3d& coeffs);

// ------------ Frene ------------
Pose getFreneByCirc(const V3d& pt0, const V3d& ptc);

// ------------ Blade ------------

struct BladeProfile {
  QVector<V3d> cx, cv, re, le;
};
using Airfoil = QVector<BladeProfile>;

// V3d jsonValueToVec3(const QJsonValue& v);
// QVector<V3d> jsonArrayToProfile(const QJsonArray& arr);
// BladeProfile jsonObjectToBladeProfile(const QJsonObject& obj);
// Airfoil parseBladeJson(QFile& f);

Pose getCxCvStartFrenet(const QVector<V3d>& cx, double L, const Pose& frenet);
Pose getCxCvEndFrenet(const QVector<V3d>& cx, double L, const Pose& frenet);
Pose getCxCvFrenet(V3d pt, const V3d& poly, const V3d& v0);
QVector<Pose> getCxCvFrenets(const QVector<V3d>& cx, const QVector<V3d>& cx_next, double L);

// ------------ Base ------------
V6d getBeltFrame(const V3d& o,
                 const Eigen::Ref<const Eigen::VectorXd>& x,
                 const Eigen::Ref<const Eigen::VectorXd>& y,
                 const Eigen::Ref<const Eigen::VectorXd>& z);

// ------------ Rsi Trajectory ------------
struct MotionParams {
  double v, a;
};

namespace rsi {
QVector<V6d> polyline(const QVector<V6d> &ref_points, const MotionParams& mp, int decimals = 3);
QVector<V6d> lin(const V6d& P1, const V6d& P2, const MotionParams& mp, int decimals = 3);
};

QVector<Pose> pathFromSurfPoses(const QVector<Pose>& surf_poses, const M4d& AiT);

QVector<V6d> posesToFrames(const QVector<Pose>& poses);

void writeOffsetsToJson(const QVector<V6d>& offsets, const QString& filePath, int decimals = 3);
#endif // PATHPLANNER_H
