#include "trajectories/timedmotion.h"

#include "jointsegments.h"

#include <algorithm>
#include <cmath>

std::optional<std::pair<V6d, qsizetype>> TimedMotion::evaluateJoints(double seconds) const
{
  const auto& points = m_data.points;
  const auto& segments = m_data.segments;
  if (!std::isfinite(seconds) || segments.isEmpty() || m_data.phases.isEmpty()) return std::nullopt;
  const double time = std::clamp(seconds, 0.0, duration());
  auto segment = segments.cbegin();
  while (segment + 1 != segments.cend() && time > points[segment->last].time) ++segment;
  qsizetype low = segment->first;
  qsizetype high = segment->last;
  while (low + 1 < high) {
    const auto middle = low + (high - low) / 2;
    if (points[middle].time <= time) low = middle;
    else high = middle;
  }
  const auto& a = points[low];
  const auto& b = points[high];
  // Stored knots return their exact IK values, including full-turn phase boundaries.
  V6d joints = time == b.time ? b.joints : a.joints;
  if (time != a.time && time != b.time) {
    const double fraction = (time - a.time) / (b.time - a.time);
    if (segment->interpolation == MotionInterpolation::JointLinear) {
      joints = a.joints + fraction * (b.joints - a.joints);
    } else if (segment->interpolation == MotionInterpolation::Smoothstep) {
      const auto& first = points[segment->first];
      const auto& last = points[segment->last];
      const double u = (time - first.time) / (last.time - first.time);
      joints = first.joints + smoothProgress(u) * (last.joints - first.joints);
    } else {
      const qsizetype interval = segment->spline + low - segment->first;
      if (segment->spline < 0 || interval >= m_data.splines.size()) return std::nullopt;
      const auto& coefficients = m_data.splines[interval];
      joints = ((coefficients[3] * fraction + coefficients[2]) * fraction + coefficients[1]) * fraction
          + coefficients[0];
    }
  }
  const qsizetype point = time == a.time ? low : high;
  qsizetype phase = 0;
  while (phase + 1 < m_data.phases.size() && m_data.phases[phase].last < point) ++phase;
  if (!withinLimits(joints, m_robot.model)) return std::nullopt;
  return std::pair{joints, phase};
}
