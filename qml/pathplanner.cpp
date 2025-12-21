#include "pathplanner.h"

// ------------ Math ------------
M4d trMatrix4x4(const V3d& delta)
{
  M4d T = M4d::Identity();
  T.block<3,1>(0,3) = delta;
  return T;
}

M4d rotMatrix4x4(double angleDeg, char axis)
{
  M4d R = M4d::Identity();
  const double ang = angleDeg * M_PI / 180.0;
  const double c = std::cos(ang), s = std::sin(ang);
  switch (axis) {
    case 'x': R(1,1)=c; R(1,2)=-s; R(2,1)= s; R(2,2)=c; break;
    case 'y': R(0,0)=c; R(0,2)= s; R(2,0)=-s; R(2,2)=c; break;
    case 'z': R(0,0)=c; R(0,1)=-s; R(1,0)= s; R(1,1)=c; break;
  }
  constexpr double EPS = 1e-4;
  R = R.unaryExpr([](double v){ return std::abs(v) <= EPS ? 0.0 : v; });
  return R;
}

V3d axisVec(char axis, double value)
{
  switch (axis) {
    case 'x': return { value, 0.0,   0.0 };
    case 'y': return { 0.0,   value, 0.0 };
    case 'z': return { 0.0,   0.0,   value };
    default:
      throw std::invalid_argument("axis must be one of: 'x','y','z'");
  }
}

Plane pointsToPlane(const Eigen::Ref<const Eigen::VectorXd>& x,
                    const Eigen::Ref<const Eigen::VectorXd>& y,
                    const Eigen::Ref<const Eigen::VectorXd>& z)
{
  const double n = static_cast<double>(x.size());
  M3d U;
  U << x.squaredNorm(), x.dot(y),        x.sum(),
       x.dot(y),        y.squaredNorm(), y.sum(),
       x.sum(),         y.sum(),         n;
  V3d V(x.dot(z), y.dot(z), z.sum());

  const auto qr = U.colPivHouseholderQr();
  const V3d P = qr.solve(V);

  Plane plane;
  plane.AA = P[0]; plane.BB = P[1]; plane.DD = P[2];
  const double denom = plane.AA*plane.AA + plane.BB*plane.BB + 1.0;
  plane.C = std::sqrt(1.0 / denom);
  plane.A = -plane.AA * plane.C;
  plane.B = -plane.BB * plane.C;
  plane.D = -plane.DD * plane.C;
  return plane;
}

V3d poly(const V3d& p0, const V3d& p1, const V3d& p2)
{
  M3d A;
  A << p0.x()*p0.x(), p0.x(), 1.0,
       p1.x()*p1.x(), p1.x(), 1.0,
       p2.x()*p2.x(), p2.x(), 1.0;

  return A.colPivHouseholderQr().solve(V3d(p0.y(), p1.y(), p2.y()));
}

EulerSolution rot2euler(const M3d &R, bool is_deg)
{
  const double B1 = -std::asin(R(2,0));
  const double B2 = M_PI + std::asin(R(2,0));

  const double cB1 = std::cos(B1);
  const double cB2 = std::cos(B2);

  const double C1 = std::atan2(R(2,1) / cB1, R(2,2) / cB1);
  const double C2 = std::atan2(R(2,1) / cB2, R(2,2) / cB2);

  const double A1 = std::atan2(R(1,0) / cB1, R(0,0) / cB1);
  const double A2 = std::atan2(R(1,0) / cB2, R(0,0) / cB2);

  const double k = is_deg ? (180.0 / M_PI) : 1.0;

  return { k*A1, k*A2, k*B1, k*B2, k*C1, k*C2 };
}

M3d euler2rot(double A, double B, double C, bool is_deg)
{
  const double k = is_deg ? (M_PI / 180.0) : 1.0;
  M3d R =
      (Eigen::AngleAxisd(A*k, V3d::UnitZ()) *
       Eigen::AngleAxisd(B*k, V3d::UnitY()) *
       Eigen::AngleAxisd(C*k, V3d::UnitX())).toRotationMatrix();
  double eps = 1e-4;
  R = R.unaryExpr([&](double v) { return std::abs(v) <= eps ? 0.0 : v; });
  return R;
}

V3d prjPointToLine(const V3d &l0, const V3d &v, const V3d &p)
{
  const double vv = v.squaredNorm();
  const double t = (p - l0).dot(v) / vv;
  return l0 + t * v;
}

V3d prjToPerpPlane(const V3d &vec, const V3d &n)
{
  V3d v = vec - vec.dot(n) * n;
  const double nn = v.norm();
  return v / nn;
}

V3d tanByPoly(const V3d &p, const V3d &coeffs)
{
  const double a0 = coeffs[0];
  const double a1 = coeffs[1];

  const double dy_dx = 2.0 * a0 * p.x() + a1;

  V3d t(1.0, dy_dx, 0.0);
  t.normalize();
  if (t.x() < 0.0) t = -t;
  return t;
}

// ------------ Frene ------------

Pose getFreneByCirc(const V3d &pt0, const V3d &ptc)
{
  V3d v = pt0 - ptc;
  V3d n = v.normalized();    // radial
  V3d t(-n.y(), n.x(), 0.0); // tangent in XY plane
  if (t.x() < 0.0) t = -t;
  V3d b = n.cross(t);

  return Pose::fromAxes(t, b, n, pt0);;
}

// ------------ Blade ------------
V3d jsonValueToVec3(const QJsonValue &v)
{
  const QJsonArray a = v.toArray(); // main-case: [x,y,z]
  return V3d(a[0].toDouble(), a[1].toDouble(), a[2].toDouble());
}

QVector<V3d> jsonArrayToProfile(const QJsonArray &arr)
{
  QVector<V3d> out;
  out.reserve(arr.size());
  for (const QJsonValue& v : arr) {
    out.push_back(jsonValueToVec3(v));
  }
  return out;
}

BladeProfile jsonObjectToBladeProfile(const QJsonObject &obj)
{
  BladeProfile bp;
  bp.cx = jsonArrayToProfile(obj.value("cx").toArray());
  bp.cv = jsonArrayToProfile(obj.value("cv").toArray());
  bp.re = jsonArrayToProfile(obj.value("re").toArray());
  bp.le = jsonArrayToProfile(obj.value("le").toArray());
  return bp;
}

Airfoil loadBladeJson(const QString& filePath)
{
  QFile f(filePath);
  f.open(QIODevice::ReadOnly);
  const QByteArray bytes = f.readAll();

  const QJsonDocument doc = QJsonDocument::fromJson(bytes);
  const QJsonArray top = doc.array();

  Airfoil airfoil;
  airfoil.reserve(top.size());

  for (const QJsonValue& v : top) {
    airfoil.push_back(jsonObjectToBladeProfile(v.toObject()));
  }
  return airfoil;
}

Pose getCxCvStartFrenet(const QVector<V3d>& cx, double L, const Pose& frenet)
{
  const V3d& pt0 = cx[0];
  const V3d& pt1 = cx[1];
  const V3d& pt = pt0 + L * (pt0 - pt1).normalized();
  return Pose::fromAxes(frenet.t, frenet.b, frenet.n, pt);
}

Pose getCxCvEndFrenet(const QVector<V3d>& cx, double L, const Pose& frenet)
{
  const int n = cx.size();
  const V3d& ptn_1 = cx[n - 2];
  const V3d& ptn   = cx[n - 1];
  const V3d& pt = ptn + L * (ptn - ptn_1).normalized();
  return Pose::fromAxes(frenet.t, frenet.b, frenet.n, pt);
}

Pose getCxCvFrenet(V3d pt, const V3d& poly, const V3d& v0) {
  V3d t = tanByPoly(pt, poly);
  V3d tanv = (v0 - pt).normalized();
  V3d n = t.cross(tanv).normalized();
  V3d b = n.cross(t).normalized();
  return Pose::fromAxes(t, b, n, pt);
}


QVector<Pose> getCxCvFrenets(const QVector<V3d>& cx, const QVector<V3d>& cx_next, double L)
{
  int n = cx.size();

  QVector<Pose> frenets;
  frenets.reserve(n + 2);

  V3d coef0 = poly(cx[0], cx[1], cx[2]);
  Pose frenet0 = getCxCvFrenet(cx[0], coef0, cx_next[0]);

  Pose startFrenet = getCxCvStartFrenet(cx, L, frenet0);

  frenets << startFrenet << frenet0;

  for (int j=1; j < n-2; j++) {
    V3d coef = poly(cx[j-1], cx[j], cx[j+1]);
    Pose frenet = getCxCvFrenet(cx[j], coef, cx_next[j]);
    frenets.push_back(frenet);
  }

  V3d coefn = poly(cx[n-3], cx[n-2], cx[n-1]);
  Pose nfrenet = getCxCvFrenet(cx[n-1], coefn, cx_next[n-1]);

  Pose endFrenet = getCxCvEndFrenet(cx, L, nfrenet);

  frenets << nfrenet << endFrenet;

  return frenets;
}

// ------------ Base ------------
V6d getBeltFrame(const V3d& o,
                 const Eigen::Ref<const Eigen::VectorXd>& x,
                 const Eigen::Ref<const Eigen::VectorXd>& y,
                 const Eigen::Ref<const Eigen::VectorXd>& z)
{
  const Plane pl = pointsToPlane(x,y,z);
  V3d n(pl.A,pl.B,pl.C);
  n.normalize();

  const V3d helper = (std::abs(n.x()) < 0.9) ? V3d::UnitX() : V3d::UnitY();
  V3d t = helper - helper.dot(n)*n;
  t.normalize();
  t = -t;

  V3d b = n.cross(t).normalized();
  t = b.cross(n);

  M4d T = M4d::Identity();
  T.block<3,1>(0,0)=t;
  T.block<3,1>(0,1)=b;
  T.block<3,1>(0,2)=n;
  T.block<3,1>(0,3)=o;

  const double A = std::atan2(T(1,0), T(0,0)) * 180.0/M_PI;
  const double B = std::asin(-T(2,0)) * 180.0/M_PI;
  const double C = std::atan2(T(2,1), T(2,2)) * 180.0/M_PI;

  return { o.x(), o.y(), o.z(), A, B, C };
}


// ------------ Cylinder ------------
Cylinder Cylinder::fromAxis(const V3d &u, const V3d &pc,
                            double R, double L, char axis)
{
  V3d y = u.normalized();

  // choose helper not parallel to y
  const V3d helper =
      (std::abs(y.x()) < 0.9) ? V3d::UnitX() : V3d::UnitY();

  V3d x = helper - helper.dot(y) * y; // remove y component
  x.normalize();

  V3d z = x.cross(y).normalized();    // right-handed: x × y = z

  M4d transform = M4d::Identity();
  transform.block<3,3>(0,0) << x, y, z;           // columns
  transform.block<3,1>(0,3) = pc;

  EulerSolution angles = rot2euler(transform.topLeftCorner<3,3>(), true);

  V6d frame;
  frame << pc.x(), pc.y(), pc.z(), angles.A1, angles.B1, angles.C1;

  Pose pose = { frame, transform };
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

  EulerSolution angles = rot2euler(transform.topLeftCorner<3,3>(), true);

  V6d frame;
  frame << o.x(), o.y(), o.z(), angles.A1, angles.B1, angles.C1;

  Pose pose = { frame, transform };
  return { R, L, pose };

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
  surf_pose.T = returnLocal ? T_local : pose.T * T_local;

  const V3d os = surf_pose.T.block<3,1>(0,3);
  EulerSolution eul = rot2euler(surf_pose.T.topLeftCorner<3,3>(), true);

  surf_pose.frame << os.x(), os.y(), os.z(), eul.A1, eul.B1, eul.C1;
  return surf_pose;
}

QVector<Pose> Cylinder::surfaceRing(int n, double L) const
{
  QVector<Pose> poses;
  poses.reserve(n);

  for (int k = 0; k < n; ++k) {
    const double angleDeg = 360.0 * double(k) / double(n);
    Pose p = surfacePose('z', L,
                         'z', angleDeg,
                         'y', -R,
                         /*returnLocal=*/true);
    poses.push_back(p);
  }
  return poses;
}

// ------------ Rsi Trajectory ------------
namespace rsi {
  QVector<V6d> polyline(const QVector<V6d>& ref_points, const MotionParams& mp, int decimals)
  {
    // [v] = mm/s; [a] = mm/s^2
    const double dt = 0.004; // 4 ms
    const int n = ref_points.size();

    // 1) cumulative arc length
    QVector<double> cumLen(n);
    cumLen[0] = 0.0;
    for (int i = 1; i < n; ++i)
      cumLen[i] = cumLen[i - 1] + (ref_points[i] - ref_points[i - 1]).norm();

    const double totalLen = cumLen.last(); // S

    const double v_max = mp.v;
    const double a     = mp.a;

    // 2) trapezoidal profile parameters (assuming constant velocity is reached)
    const double t_acc = v_max / a;                 // accel time
    const double t_dec = t_acc;                     // decel time (symmetric)
    const double s_acc = 0.5 * a * t_acc * t_acc;   // distance in accel
    const double s_dec = s_acc;                     // distance in decel
    const double s_const = totalLen - s_acc - s_dec;
    const double t_const = s_const / v_max;
    const double T_total = t_acc + t_const + t_dec;

    const int steps = static_cast<int>(std::ceil(T_total / dt));

    QVector<V6d> offsets;
    offsets.reserve(steps);

    V6d prevPos = ref_points.front();

    const double scale = std::pow(10.0, decimals);

    for (int k = 1; k <= steps; ++k) {
      double t = k * dt;
      if (t > T_total) t = T_total;

      // 3) s(t) along contour
      double s;
      if (t <= t_acc) {
        // acceleration
        s = 0.5 * a * t * t;
      } else if (t <= t_acc + t_const) {
        // constant speed
        const double t2 = t - t_acc;
        s = s_acc + v_max * t2;
      } else {
        // deceleration
        const double t3 = t - t_acc - t_const;
        s = s_acc + s_const + v_max * t3 - 0.5 * a * t3 * t3;
      }
      if (s > totalLen)
        s = totalLen;

      // 4) find segment for this s
      auto it = std::upper_bound(cumLen.begin(), cumLen.end(), s);
      int idx = int(std::distance(cumLen.begin(), it)) - 1;
      if (idx < 0)      idx = 0;
      if (idx >= n - 1) idx = n - 2;

      const double segStart = cumLen[idx];
      const double segLen   = cumLen[idx + 1] - segStart;
      const double alpha    = segLen > 0.0 ? (s - segStart) / segLen : 0.0;

      // 5) interpolate pose
      V6d currPos = (1.0 - alpha) * ref_points[idx] + alpha * ref_points[idx + 1];

      // 6) offset and rounding
      V6d dP = currPos - prevPos;
      for (int i = 0; i < 6; ++i)
        dP(i) = std::round(dP(i) * scale) / scale;

      offsets.push_back(dP);
      prevPos = currPos;
    }

    return offsets;
  }

  QVector<V6d> lin(const V6d& P1, const V6d& P2, const MotionParams &mp, int decimals)
  {
    const double dt = 0.004;          // 4 ms
    const V6d d   = P2 - P1;
    const double L  = d.norm();       // total contour length

    if (L <= 0.0) return {};          // no motion

    // Normalize direction in 6D
    const V6d dir = d / L;

    double v = mp.v;
    double a = mp.a;

    // --- Trapezoidal / triangular profile in 1D (arc length) ---
    double t_acc = v / a;
    double s_acc = 0.5 * a * t_acc * t_acc;

    // If we can't reach v_max -> triangular profile
    if (2.0 * s_acc > L) {
      t_acc = std::sqrt(L / a);
      s_acc = 0.5 * a * t_acc * t_acc;
      v     = a * t_acc; // peak velocity for triangular
    }

    const double t_dec   = t_acc;
    const double s_dec   = s_acc;
    const double s_const = L - s_acc - s_dec;
    const double t_const = s_const > 0.0 ? s_const / v : 0.0;
    const double T_total = t_acc + t_const + t_dec;

    const int steps = static_cast<int>(std::ceil(T_total / dt));

    QVector<V6d> offsets;
    offsets.reserve(steps);

    V6d prevPos = P1;
    const double scale = std::pow(10.0, decimals);

    for (int k = 1; k <= steps; ++k) {
      double t = k * dt;
      if (t > T_total) t = T_total;

      // s(t) along the line
      double s;
      if (t <= t_acc) {
        s = 0.5 * a * t * t;                              // accel
      } else if (t <= t_acc + t_const) {
        double t2 = t - t_acc;
        s = s_acc + v * t2;                               // const
      } else {
        double t3 = t - t_acc - t_const;
        s = s_acc + s_const + v * t3 - 0.5 * a * t3 * t3; // decel
      }
      if (s > L) s = L;

      V6d currPos = P1 + dir * s;
      V6d dP      = currPos - prevPos;

      // rounding
      for (int i = 0; i < 6; ++i)
        dP(i) = std::round(dP(i) * scale) / scale;

      offsets.push_back(dP);
      prevPos = currPos;
    }

    return offsets;
  }
}

//-----------------------------------------

QVector<Pose> pathFromSurfPoses(const QVector<Pose>& surf_poses, const M4d& AiT)
{
  QVector<Pose> path;
  path.reserve(surf_poses.size());

  for (const Pose& surf_pose : surf_poses) {
    const M4d AiB = surf_pose.T;
    const M4d ABT = AiT * AiB.inverse();

    Pose out = Pose::fromTransform(ABT);
    path.push_back(out);
  }

  return path;
}

QVector<V6d> posesToFrames(const QVector<Pose>& poses)
{
  QVector<V6d> frames;
  frames.reserve(poses.size());

  for (const Pose& p : poses)
    frames.push_back(p.frame);   // [x,y,z,A,B,C] in deg

  return frames;
}

void writeOffsetsToJson(const QVector<V6d> &offsets, const QString &filePath, int decimals)
{
  QJsonArray root;

  for (const V6d &v : offsets) {
    QJsonArray row;
    for (int i = 0; i < 6; ++i) {
      QString s = QString::number(v(i), 'f', decimals);
      row.append(s);
    }
    root.append(row);
  }

  QJsonDocument doc(root);

  QFile file(filePath);
  file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();
}





