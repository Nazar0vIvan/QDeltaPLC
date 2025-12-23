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

// ------------ Pose ------------

Pose Pose::fromFrame(const V6d &frame_)
{
  Pose out;
  out.frame = frame_;
  out.syncTransfByFrame();
  out.syncAxesByTransf();
  return out;
}

Pose Pose::fromTransform(const M4d &T_)
{
  Pose out;
  out.transf = T_;
  out.syncAxesByTransf();
  out.syncFrameByTransf();
  return out;
}

Pose Pose::fromAxes(const V3d &t_, const V3d &b_, const V3d &n_, const V3d &p_)
{
  Pose out;
  out.t = t_;
  out.b = b_;
  out.n = n_;
  out.p = p_;
  out.syncTransfByAxes();
  out.syncFrameByTransf();
  return out;
}

void Pose::syncTransfByAxes()
{
  transf.setIdentity();
  transf.block<3,1>(0,0) = t;
  transf.block<3,1>(0,1) = b;
  transf.block<3,1>(0,2) = n;
  transf.block<3,1>(0,3) = p;
}

void Pose::syncAxesByTransf()
{
  p = transf.block<3,1>(0,3);
  const M3d R = transf.block<3,3>(0,0);
  t = R.col(0);
  b = R.col(1);
  n = R.col(2);

  const double eps = 1e-12;

  if (t.norm() > eps) t.normalize();
  if (b.norm() > eps) b.normalize();
  if (n.norm() > eps) n.normalize();
}

void Pose::syncFrameByTransf()
{
  const Eigen::Vector3d tr = transf.block<3,1>(0,3);
  const EulerSolution eul = rot2euler(transf.topLeftCorner<3,3>(), true);
  frame << tr.x(), tr.y(), tr.z(), eul.A1, eul.B1, eul.C1;
}

void Pose::syncTransfByFrame()
{
  const M3d R = euler2rot(frame(3), frame(4), frame(5), true);

  transf.setIdentity();
  transf.block<3,3>(0,0) = R;
  transf.block<3,1>(0,3) = V3d(frame(0), frame(1), frame(2));
}

// ------------ Frene ------------

Pose getFreneByCirc(const V3d &pt0, const V3d &ptc)
{
  V3d v = pt0 - ptc;
  V3d n = v.normalized();
  V3d t(-n.y(), n.x(), 0.0);
  if (t.x() < 0.0) t = -t;
  V3d b = n.cross(t);

  return Pose::fromAxes(t, b, n, pt0);
}

// ------------ Blade ------------

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

Pose getCxCvFrenet(V3d pt, const V3d& poly, const V3d& v0)
{
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


M4d rigidInverse(const M4d &T)
{
  M4d inv = M4d::Identity();
  const M3d R = T.topLeftCorner<3,3>();
  const V3d p = T.block<3,1>(0,3);
  const M3d Rt = R.transpose();
  inv.topLeftCorner<3,3>() = Rt;
  inv.block<3,1>(0,3) = -Rt * p;
  return inv;
}

double unwrapToNearest(double angDeg, double refDeg)
{
  double a = angDeg;
  while (a - refDeg > 180.0) a -= 360.0;
  while (a - refDeg < -180.0) a += 360.0;
  return a;
}

QVector<double> cumLen3(const QVector<V3d> &pts)
{
  QVector<double> s;
  s.resize(pts.size());
  if (pts.isEmpty()) return s;
  s[0] = 0.0;
  for (int i = 1; i < pts.size(); ++i)
    s[i] = s[i-1] + (pts[i] - pts[i-1]).norm();
  return s;
}

SampleAtSResult sampleAtS(const QVector<double> &s, double S)
{
  const int n = s.size();
  if (n < 2) return {0, 0.0};

  if (S <= s.front()) return {0, 0.0};
  if (S >= s.back())  return {n-2, 1.0};

  auto it = std::upper_bound(s.begin(), s.end(), S);
  int idx = int(std::distance(s.begin(), it)) - 1;
  idx = std::max(0, std::min(idx, n-2));

  const double s0 = s[idx];
  const double s1 = s[idx+1];
  const double seg = (s1 - s0);
  const double a = (seg > 1e-12) ? (S - s0) / seg : 0.0;
  return { idx, std::max(0.0, std::min(1.0, a)) };
}

V3d lerp(const V3d &a, const V3d &b, double t)
{
  return (1.0 - t) * a + t * b;
}

void enforceFrameContinuity(bool hasPrev, const V3d &tPrev, const V3d &nPrev, V3d &t, V3d &n)
{
  if (!hasPrev) return;

  // 1) keep tangent direction consistent (flip t and n together -> preserves det)
  if (tPrev.dot(t) < 0.0) {
    t = -t;
    n = -n;
  }

  // 2) keep normal consistent (flip n only; then b will flip accordingly)
  if (nPrev.dot(n) < 0.0) {
    n = -n;
  }
}

Pose frenetFromGeom(const V3d &p, const V3d &t_hint, const V3d &span_to_next, bool hasPrev, const V3d &tPrev, const V3d &nPrev)
{
  V3d t = t_hint;
  const double tn = t.norm();
  if (tn > 1e-12) t /= tn; else t = V3d::UnitX();

  V3d v = span_to_next;
  const double vn = v.norm();
  if (vn > 1e-12) v /= vn; else v = V3d::UnitZ();

  V3d n = t.cross(v);
  const double nn = n.norm();
  if (nn > 1e-12) n /= nn;
  else {
    // fallback: keep previous normal if degenerate
    n = hasPrev ? nPrev : V3d::UnitZ();
    const double nnn = n.norm();
    if (nnn > 1e-12) n /= nnn;
  }

  enforceFrameContinuity(hasPrev, tPrev, nPrev, t, n);

  // right-handed completion
  V3d b = n.cross(t);
  const double bn = b.norm();
  if (bn > 1e-12) b /= bn;

  // re-orthonormalize n to remove numeric drift
  n = t.cross(b);
  const double n2 = n.norm();
  if (n2 > 1e-12) n /= n2;

  return Pose::fromAxes(t, b, n, p);
}

void deltaRotToEulerZYX_deg(const M3d &dR, double &dA, double &dB, double &dC)
{
  const EulerSolution e = rot2euler(dR, true);

  // rot2euler returns two branches; for delta we want the one closer to 0
  const double s1 = std::abs(e.A1) + std::abs(e.B1) + std::abs(e.C1);
  const double s2 = std::abs(e.A2) + std::abs(e.B2) + std::abs(e.C2);

  if (s1 <= s2) { dA = e.A1; dB = e.B1; dC = e.C1; }
  else          { dA = e.A2; dB = e.B2; dC = e.C2; }
}

QVector<V6d> offsetsFromCxContour(const QVector<V3d> &cx, const QVector<V3d> &cx_next, const M4d &AiT, const MotionParams &mp, int decimals)
{
  QVector<V6d> out;
  if (cx.size() < 2 || cx_next.size() != cx.size()) return out;

  const double dt = 0.004;           // RSI cycle [s]
  const double V  = mp.v;            // [mm/s]
  const double a  = mp.a;            // [mm/s^2]

  // distance needed to reach V with accel a: s = V^2/(2a)
  const double s_acc = (a > 1e-12) ? (0.5 * V * V / a) : 0.0;
  const double s_dec = s_acc;

  // --- build StartPoint / EndPoint extensions (same idea as your getCxCvStart/EndFrenet) ---
  const V3d p0   = cx.front();
  const V3d p1   = cx[1];
  const V3d pn   = cx.back();
  const V3d pn_1 = cx[cx.size() - 2];

  const V3d dirStart = safeUnit(p0 - p1, V3d::UnitX());
  const V3d dirEnd   = safeUnit(pn - pn_1, V3d::UnitX());

  const V3d startP = p0 + s_acc * dirStart;
  const V3d endP   = pn + s_dec * dirEnd;

  const V3d q0   = cx_next.front();
  const V3d q1   = cx_next[1];
  const V3d qn   = cx_next.back();
  const V3d qn_1 = cx_next[cx_next.size() - 2];

  const V3d dirStartN = safeUnit(q0 - q1, V3d::UnitX());
  const V3d dirEndN   = safeUnit(qn - qn_1, V3d::UnitX());

  const V3d startQ = q0 + s_acc * dirStartN;
  const V3d endQ   = qn + s_dec * dirEndN;

  QVector<V3d> pts;      pts.reserve(cx.size() + 2);
  QVector<V3d> ptsNext;  ptsNext.reserve(cx.size() + 2);

  pts     << startP;
  ptsNext << startQ;
  for (int i = 0; i < cx.size(); ++i) {
    pts     << cx[i];
    ptsNext << cx_next[i];
  }
  pts     << endP;
  ptsNext << endQ;

  const QVector<double> S = cumLen3(pts);
  const double totalLen = S.isEmpty() ? 0.0 : S.back();
  if (totalLen <= 1e-9) return out;

  // precompute smooth vertex tangents (THIS is the critical fix vs. segment t_hint)
  const QVector<V3d> tV = vertexTangents(pts);

  // trapezoid timing (extension ensures we can reach V)
  const double t_acc = (a > 1e-12) ? (V / a) : 0.0;
  const double s_const = std::max(0.0, totalLen - s_acc - s_dec);
  const double t_const = (V > 1e-12) ? (s_const / V) : 0.0;
  const double T_total = t_acc + t_const + t_acc;

  const int steps = std::max(1, int(std::ceil(T_total / dt)));
  out.reserve(steps);

  // initial pose at s = 0
  const V3d p_init = pts[0];
  const V3d q_init = ptsNext[0];

  Pose prevAiB = frenetFromGeom(p_init,
                                tV[0],
                                (q_init - p_init),
                                /*hasPrev=*/false,
                                V3d::Zero(),
                                V3d::Zero());

  M4d prevABT = AiT * rigidInverse(prevAiB.transf);

  bool hasPrevFrame = true;
  V3d tPrev = prevAiB.t;
  V3d nPrev = prevAiB.n;

  const double scale = std::pow(10.0, decimals);

  for (int k = 1; k <= steps; ++k) {
    double t = k * dt;
    if (t > T_total) t = T_total;

    // arc-length law s(t)
    double s;
    if (t <= t_acc) {
      s = 0.5 * a * t * t;
    } else if (t <= t_acc + t_const) {
      const double t2 = t - t_acc;
      s = s_acc + V * t2;
    } else {
      const double t3 = t - t_acc - t_const;
      s = s_acc + s_const + V * t3 - 0.5 * a * t3 * t3;
    }
    if (s > totalLen) s = totalLen;

    // sample along extended contour
    const SampleAtSResult smp = sampleAtS(S, s);
    const int i = smp.idx;
    const double u = smp.alpha;

    const V3d p = lerp(pts[i],     pts[i + 1],     u);
    const V3d q = lerp(ptsNext[i], ptsNext[i + 1], u);

    // smooth tangent hint (instead of raw segment direction)
    const V3d t_hint = blendTangentAligned(tV[i], tV[i + 1], u);

    Pose AiB_pose = frenetFromGeom(p, t_hint, (q - p),
                                   hasPrevFrame, tPrev, nPrev);

    tPrev = AiB_pose.t;
    nPrev = AiB_pose.n;
    hasPrevFrame = true;

    const M4d ABT = AiT * rigidInverse(AiB_pose.transf);

    // delta between successive ABT
    const M4d dT = rigidInverse(prevABT) * ABT;
    const V3d dp = dT.block<3,1>(0,3);

    double dA = 0.0, dB = 0.0, dC = 0.0;
    deltaRotToEulerZYX_deg(dT.topLeftCorner<3,3>(), dA, dB, dC);

    V6d dP;
    dP << dp.x(), dp.y(), dp.z(), dA, dB, dC;

    for (int j = 0; j < 6; ++j)
      dP(j) = std::round(dP(j) * scale) / scale;

    out.push_back(dP);
    prevABT = ABT;
  }

  return out;
}

V3d safeUnit(const V3d &v, const V3d &fallback, double eps)
{
  const double n = v.norm();
  return (n > eps) ? (v / n) : fallback;
}

QVector<V3d> vertexTangents(const QVector<V3d> &pts)
{
  const int m = pts.size();
  QVector<V3d> tv;
  tv.resize(m);

  if (m == 0) return tv;
  if (m == 1) { tv[0] = V3d::UnitX(); return tv; }

  for (int j = 0; j < m; ++j) {
    V3d v;
    if (j == 0) {
      v = pts[1] - pts[0];
    } else if (j == m - 1) {
      v = pts[m - 1] - pts[m - 2];
    } else {
      v = pts[j + 1] - pts[j - 1];
      if (v.norm() < 1e-9) v = pts[j + 1] - pts[j];
      if (v.norm() < 1e-9) v = pts[j] - pts[j - 1];
    }
    tv[j] = safeUnit(v, V3d::UnitX());
  }

  return tv;
}

V3d blendTangentAligned(const V3d &t0_in, const V3d &t1_in, double u)
{
  V3d t0 = t0_in;
  V3d t1 = t1_in;
  if (t0.dot(t1) < 0.0) t1 = -t1;
  V3d t = (1.0 - u) * t0 + u * t1;
  return safeUnit(t, t0);
}

} // namespace


// ----------------------------------------

QVector<Pose> pathFromSurfPoses(const QVector<Pose>& surf_poses, const M4d& AiT)
{
  QVector<Pose> path;
  path.reserve(surf_poses.size());

  for (const Pose& surf_pose : surf_poses) {
    const M4d AiB = surf_pose.transf;
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






