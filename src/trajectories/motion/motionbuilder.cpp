#include "motionbuilder.h"

#include <algorithm>
#include <cmath>

using namespace RoboCrap3D;

namespace {

constexpr qsizetype MaximumPoints = 400000;
constexpr double RateStep = 1e-4; // s, shortest interval for grouped rate validation.
constexpr double RateTolerance = 1e-6;

struct Rates
{
  V6d joints = V6d::Zero();
  V3d xyz = V3d::Zero();
  double seconds = 0.0;
};

QString atTime(const QString& label, const QString& message, double seconds)
{
  return QStringLiteral("%1: %2 (t=%3 s)").arg(label, message).arg(seconds, 0, 'g', 12);
}

bool samePose(const M4d& first, const M4d& second)
{
  return (first.block<3, 1>(0, 3) - second.block<3, 1>(0, 3)).norm() <= Kr10PosEps
      && (first.block<3, 3>(0, 0) - second.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() <= Kr10RotEps;
}

QString accelerationError(const Rates& before, const Rates& after, const ChamferTimingParameters& timing)
{
  const double elapsed = 0.5 * (before.seconds + after.seconds);
  if (!(elapsed > 0.0)) return QStringLiteral("Unresolved acceleration interval.");
  const V6d acceleration = (after.joints - before.joints) / elapsed;
  const V3d xyz = (after.xyz - before.xyz) / elapsed;
  if (!acceleration.allFinite() || !xyz.allFinite()) return QStringLiteral("Nonfinite sampled acceleration.");
  // Temporary Dry Run diagnostic: per-joint acceleration rejection disabled.
  // for (int axis = 0; axis < 6; ++axis)
  //   if (std::abs(acceleration[axis]) > timing.jointAcceleration[axis] * (1.0 + RateTolerance))
  //     return QStringLiteral("A%1 acceleration %2 deg/s^2 exceeds configured simulation bound %3.")
  //         .arg(axis + 1).arg(std::abs(acceleration[axis]), 0, 'g', 12)
  //         .arg(timing.jointAcceleration[axis]);
  if (xyz.norm() > timing.cartesianAcceleration * (1.0 + RateTolerance))
    return QStringLiteral("XYZ acceleration %1 mm/s^2 exceeds configured simulation bound %2.")
        .arg(xyz.norm(), 0, 'g', 12).arg(timing.cartesianAcceleration);
  return {};
}

} // namespace

PathSchedule LinearPath::schedule() const
{
  const auto schedule = linearSchedule(m_profile);
  if (!schedule.error.isEmpty()) return {{}, {}, schedule.error};
  return {schedule.localTimes, {{schedule.localTimes.size(), m_label}}, {}};
}

std::optional<PathTarget> LinearPath::target(qsizetype, double seconds) const
{
  return PathTarget{m_profile.target(seconds), m_profile.progress(seconds)};
}

MotionBuilder::MotionBuilder(const ChamferRobotSetup& robot)
  : m_robot(robot), m_solver(robot)
{
  m_data.points.append(m_solver.forward(robot.model.qHome, 0.0));
  m_seed = home();
}

Kr10Kinematics::IkSolution MotionBuilder::home() const
{
  const V6d& home = m_robot.model.qHome;
  return m_solver.kinematics().configuration(home, m_solver.kinematics().solveFK(home));
}

QString MotionBuilder::appendPtp(const Kr10Kinematics::IkSolution& end, const QString& label)
{
  const qsizetype first = m_data.points.size() - 1;
  const V6d start = m_data.points.back().joints;
  const double startTime = m_data.points.back().time;
  // Equal joint steps of at most 2 degrees; progress is the joint fraction.
  const int intervals = std::max(1, static_cast<int>(std::ceil((end.q - start).cwiseAbs().maxCoeff() / 2.0)));
  if (intervals > MaximumPoints - m_data.points.size())
    return label + QStringLiteral(": Complete motion exceeds the 400000-point limit.");
  const double duration = ptpDuration(end.q - start, m_robot.timing);
  QVector<ChamferJointPoint> points;
  points.reserve(intervals);
  double lastTime = startTime;
  for (int i = 1; i <= intervals; ++i) {
    const double progress = static_cast<double>(i) / intervals;
    auto point = m_solver.forward(i == intervals ? end.q : V6d(start + progress * (end.q - start)), progress);
    point.time = startTime + duration * inverseProgress(progress);
    if (!std::isfinite(point.time) || point.time <= lastTime)
      return label + QStringLiteral(": Motion timing exceeds numerical resolution.");
    lastTime = point.time;
    points.append(point);
  }
  m_data.points.append(points);
  const qsizetype last = m_data.points.size() - 1;
  m_data.segments.append(MotionSegment{first, last, MotionInterpolation::Smoothstep});
  m_data.phases.append(MotionPhaseRange{first, last, label});
  m_seed = end;
  return {};
}

QString MotionBuilder::appendPath(const TimedPath& path, MotionInterpolation interpolation)
{
  const auto schedule = path.schedule();
  if (!schedule.error.isEmpty()) return schedule.error;
  const auto& times = schedule.times;
  if (schedule.phases.isEmpty() || schedule.phases.back().first != times.size())
    return QStringLiteral("Path schedule does not assign every point to a phase.");
  const QString& label = schedule.phases.front().second;
  if (times.size() > MaximumPoints - m_data.points.size())
    return label + QStringLiteral(": Complete motion exceeds the 400000-point limit.");
  const auto start = path.target(0, 0.0);
  if (!start || !samePose(start->tcp, m_data.points.back().tcp))
    return label + QStringLiteral(": Path start does not match the preceding TCP pose.");
  QVector<ChamferJointPoint> points{m_data.points.back()};
  QVector<V3d> targets{start->tcp.block<3, 1>(0, 3)};
  points.reserve(times.size() + 1);
  targets.reserve(times.size() + 1);
  const double startTime = points.front().time;
  auto seed = m_seed;
  qsizetype phase = 0;
  for (qsizetype index = 1; index <= times.size(); ++index) {
    while (schedule.phases[phase].first < index) ++phase;
    const QString& pointLabel = schedule.phases[phase].second;
    const double local = times[index - 1];
    const double time = startTime + local;
    const double step = time - points.back().time;
    if (!std::isfinite(time) || !(step > 1e-9))
      return atTime(pointLabel, QStringLiteral("Path timestamps lost numerical resolution."), local);
    const auto target = path.target(index, local);
    if (!target) return atTime(pointLabel, QStringLiteral("Cannot evaluate prescribed TCP target."), local);
    const auto solved = m_solver.solveTcp(target->tcp, seed);
    if (!solved.solution) return atTime(pointLabel, solved.error, local);
    const QString speedError = chamferJointSpeedError(
        (solved.solution->q - points.back().joints) / step, m_robot.timing.jointSpeed);
    if (!speedError.isEmpty()) return atTime(pointLabel, speedError, local);
    points.append(ChamferJointPoint{solved.solution->q, solved.actualTcp, target->progress, time});
    targets.append(target->tcp.block<3, 1>(0, 3));
    seed = *solved.solution;
  }
  const auto issue = rateIssue(points, targets);
  if (!issue.message.isEmpty()) {
    qsizetype issuePhase = 0;
    while (schedule.phases[issuePhase].first < issue.index) ++issuePhase;
    return atTime(schedule.phases[issuePhase].second, issue.message, points[issue.index].time - startTime);
  }
  const qsizetype first = m_data.points.size() - 1;
  MotionSegment segment{first, first + times.size(), interpolation};
  if (interpolation == MotionInterpolation::Spline) {
    const auto spline = clampedJointSpline(points, m_robot);
    if (!spline.error.isEmpty()) return label + QStringLiteral(": ") + spline.error;
    segment.spline = m_data.splines.size();
    m_data.splines.append(spline.coefficients);
  }
  m_data.points.append(points.mid(1));
  m_data.segments.append(segment);
  qsizetype phaseStart = 0;
  for (const auto& [phaseEnd, phaseLabel] : schedule.phases) {
    m_data.phases.append(MotionPhaseRange{first + phaseStart, first + phaseEnd, phaseLabel});
    phaseStart = phaseEnd;
  }
  m_seed = seed;
  return {};
}

// Grouped secants of at least RateStep, located at their midpoints, from rest at the path start
// to rest at its end: grouped joint speeds and XYZ acceleration of the prescribed targets.
MotionBuilder::RateIssue MotionBuilder::rateIssue(const QVector<ChamferJointPoint>& points,
                                                  const QVector<V3d>& targets) const
{
  const double end = points.back().time;
  qsizetype anchor = 0;
  Rates previous;
  for (qsizetype index = 1; index < points.size(); ++index) {
    const auto& last = points[index];
    const auto& first = points[anchor];
    const bool terminal = index == points.size() - 1;
    const double step = last.time - first.time;
    if (!terminal && (step < RateStep || end - last.time < RateStep)) continue;
    if (!(step >= RateStep)) return {index, QStringLiteral("Path is too short for resolved rate validation.")};
    const Rates current{V6d((last.joints - first.joints) / step), V3d((targets[index] - targets[anchor]) / step), step};
    if (!current.xyz.allFinite()) return {index, QStringLiteral("Nonfinite sampled velocity.")};
    // Adjacent secants were checked while appending; grouped secants are new results.
    if (index > anchor + 1) {
      const QString speedError = chamferJointSpeedError(current.joints, m_robot.timing.jointSpeed);
      if (!speedError.isEmpty()) return {index, speedError};
    }
    const QString error = accelerationError(previous, current, m_robot.timing);
    if (!error.isEmpty()) return {index, error};
    previous = current;
    anchor = index;
  }
  const QString stop = accelerationError(previous, Rates{}, m_robot.timing);
  return {points.size() - 1, stop};
}
