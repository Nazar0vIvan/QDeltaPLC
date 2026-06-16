#include "rsi.h"

#include <algorithm>
#include <cmath>

#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>

namespace {

constexpr double kDt = 0.004; // 4 ms

bool isValidMotionParams(const MotionParams& mp)
{
  return std::isfinite(mp.v) &&
         std::isfinite(mp.a) &&
         mp.v > 0.0 &&
         mp.a > 0.0;
}

int normalizedDecimals(int decimals)
{
  return std::clamp(decimals, 0, 12);
}

double roundToDecimals(double value, int decimals)
{
  const double scale = std::pow(10.0, normalizedDecimals(decimals));
  return std::round(value * scale) / scale;
}

V6d roundedOffset(const V6d& offset, int decimals)
{
  V6d rounded = offset;

  for (int i = 0; i < 6; ++i) {
    rounded(i) = roundToDecimals(rounded(i), decimals);
  }

  return rounded;
}

std::optional<MotionProfile> makeMotionProfile(double length, const MotionParams& mp)
{
  if (!std::isfinite(length) || length <= 0.0 || !isValidMotionParams(mp)) {
    return std::nullopt;
  }

  MotionProfile profile;
  profile.v = mp.v;
  profile.a = mp.a;

  profile.tAcc = profile.v / profile.a;
  profile.sAcc = 0.5 * profile.a * profile.tAcc * profile.tAcc;

  if (2.0 * profile.sAcc > length) {
    profile.tAcc = std::sqrt(length / profile.a);
    profile.sAcc = 0.5 * profile.a * profile.tAcc * profile.tAcc;
    profile.v = profile.a * profile.tAcc;
  }

  profile.sConst = length - 2.0 * profile.sAcc;
  if (profile.sConst < 0.0) {
    profile.sConst = 0.0;
  }

  profile.tConst = profile.sConst > 0.0 ? profile.sConst / profile.v : 0.0;
  profile.totalTime = 2.0 * profile.tAcc + profile.tConst;

  if (!std::isfinite(profile.totalTime) || profile.totalTime <= 0.0) {
    return std::nullopt;
  }

  return profile;
}

double distanceAtTime(const MotionProfile& profile, double time)
{
  if (time <= profile.tAcc) {
    return 0.5 * profile.a * time * time;
  }

  if (time <= profile.tAcc + profile.tConst) {
    const double t = time - profile.tAcc;
    return profile.sAcc + profile.v * t;
  }
  const double t = time - profile.tAcc - profile.tConst;

  return profile.sAcc + profile.sConst + profile.v * t - 0.5 * profile.a * t * t;
}

} // namespace

QVector<V6d> RsiPath::lin(const V6d& p1, const V6d& p2, const MotionParams& mp, int decimals)
{
  if (!p1.allFinite() || !p2.allFinite()) {
    return {};
  }

  const V6d delta = p2 - p1;
  const double length = delta.norm();

  const auto profile = makeMotionProfile(length, mp);

  if (!profile) return {};

  const V6d dir = delta / length;

  const int steps = static_cast<int>(std::ceil(profile->totalTime / kDt));

  if (steps <= 0) return {};

  QVector<V6d> offsets;
  offsets.reserve(steps);

  V6d prevPos = p1;

  for (int k = 1; k <= steps; ++k) {
    double time = static_cast<double>(k) * kDt;

    if (time > profile->totalTime) {
      time = profile->totalTime;
    }

    double s = distanceAtTime(*profile, time);

    if (s > length) s = length;

    const V6d currPos = p1 + dir * s;
    const V6d offset = roundedOffset(currPos - prevPos, decimals);

    offsets.push_back(offset);

    prevPos = currPos;
  }

  return offsets;
}

QVector<V6d> RsiPath::polyline(const QVector<V6d>& refPoints, const MotionParams& mp, int decimals)
{
  if (refPoints.size() < 2)  return {};

  const int n = refPoints.size();

  QVector<double> cumLen(n);
  cumLen[0] = 0.0;

  for (int i = 1; i < n; ++i) {
    const V6d segment = refPoints[i] - refPoints[i - 1];
    const double segmentLen = segment.norm();

    if (!std::isfinite(segmentLen)) return {};

    cumLen[i] = cumLen[i - 1] + segmentLen;
  }

  const double totalLen = cumLen.last();

  const auto profile = makeMotionProfile(totalLen, mp);

  if (!profile) return {};

  const int steps = static_cast<int>(std::ceil(profile->totalTime / kDt));

  if (steps <= 0) return {};

  QVector<V6d> offsets;
  offsets.reserve(steps);

  V6d prevPos = refPoints.front();

  for (int k = 1; k <= steps; ++k) {
    double time = static_cast<double>(k) * kDt;

    if (time > profile->totalTime) {
      time = profile->totalTime;
    }

    double s = distanceAtTime(*profile, time);

    if (s > totalLen)  s = totalLen;

    auto it = std::upper_bound(cumLen.begin(), cumLen.end(), s);

    int idx = static_cast<int>(std::distance(cumLen.begin(), it)) - 1;

    if (idx < 0) idx = 0;
    if (idx >= n - 1) idx = n - 2;

    const double segStart = cumLen[idx];
    const double segLen = cumLen[idx + 1] - segStart;
    const double alpha = segLen > 0.0 ? (s - segStart) / segLen : 0.0;

    const V6d currPos = (1.0 - alpha) * refPoints[idx] + alpha * refPoints[idx + 1];

    const V6d offset = roundedOffset(currPos - prevPos, decimals);

    offsets.push_back(offset);

    prevPos = currPos;
  }

  return offsets;
}

std::optional<QVector<Pose>> RsiPath::fromSurfPoses(const QVector<Pose>& surfPoses, const M4d& aiT)
{
  if (!aiT.allFinite()) return std::nullopt;

  QVector<Pose> path;
  path.reserve(surfPoses.size());

  for (const Pose& surfPose : surfPoses) {
    const M4d aiB = surfPose.transform();
    const M4d abT = aiT * aiB.inverse();
    const auto pose = Pose::fromTransform(abT);

    if (!pose) return std::nullopt;

    path.push_back(*pose);
  }

  return path;
}

bool writeOffsetsToJson(const QVector<V6d>& offsets, const QString& filePath, int decimals)
{
  if (filePath.isEmpty()) return false;

  QJsonArray root;

  for (const V6d& offset : offsets) {
    QJsonArray row;

    for (int i = 0; i < 6; ++i) {
      row.append(QString::number(offset(i), 'f', normalizedDecimals(decimals)));
    }

    root.append(row);
  }

  const QJsonDocument document(root);
  const QByteArray bytes = document.toJson(QJsonDocument::Indented);

  QFile file(filePath);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    return false;
  }

  return file.write(bytes) == bytes.size();
}
