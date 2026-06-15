#include "rsi.h"

RsiPath::RsiPath () {}

QVector<V6d> RsiPath::lin(const V6d& P1, const V6d& P2, const MotionParams &mp, int decimals)
{
  const double dt = 0.004;    // 4 ms
  const V6d d   = P2 - P1;
  const double L  = d.norm(); // total contour length

  if (L <= 0.0) return {};    // no motion

  // Normalize direction in 6D
  const V6d dir = d / L;

  double v = mp.v;
  double a = mp.a;

  // Trapezoidal / triangular profile in 1D (arc length) ---
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

    if (s > L) {
      s = L;
    }

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

QVector<V6d> RsiPath::polyline(const QVector<V6d>& ref_points, const MotionParams& mp, int decimals)
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
    if (s > totalLen) {
      s = totalLen;
    }

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

QVector<Pose> RsiPath::fromSurfPoses(const QVector<Pose>& surf_poses, const M4d& AiT)
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
