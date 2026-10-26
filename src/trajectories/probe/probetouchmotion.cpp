#include "trajectories/probetouchmotion.h"

#include "../chamfer/chamfertcpsolver.h"
#include "../motion/jointsegments.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

using namespace RoboCrap3D;

namespace {

constexpr qsizetype MaximumPoints = 400000;
constexpr double ClearanceTolerance = 0.0001; // mm

struct MotionStep
{
  ProbeMotionPhase phase;
  M4d target; // TCP pose at the end of the segment; unused by ReturnHome.
};

struct BuildResult
{
  ProbeTouchMotionData data;
  QString error;
};

bool ptpSegment(ProbeSegment segment)
{
  return segment == ProbeSegment::ToEntry || segment == ProbeSegment::Unwind || segment == ProbeSegment::ReturnHome;
}

// Constant-speed LIN without ramps, interpolated joint-linearly in playback.
bool touchSegment(ProbeSegment segment)
{
  return segment == ProbeSegment::Approach || segment == ProbeSegment::Retract;
}

QVector<MotionStep> motionSteps(const ProbeTouchPlan& plan)
{
  QVector<MotionStep> steps{MotionStep{{0, ProbeSegment::ToEntry}, plan.entry},
                            MotionStep{{0, ProbeSegment::Descend}, plan.touches.front().start}};
  int number = 0;
  for (const ProbeTouch& touch : plan.touches) {
    ++number;
    for (const M4d& pose : touch.transfer) steps.append(MotionStep{{number, ProbeSegment::Transfer}, pose});
    steps.append(MotionStep{{number, ProbeSegment::Approach}, touch.contact});
    steps.append(MotionStep{{number, ProbeSegment::Retract}, touch.start});
    if (touch.unwindAfter) steps.append(MotionStep{{number, ProbeSegment::Unwind}, touch.start});
  }
  steps.append(MotionStep{{0, ProbeSegment::Retreat}, plan.exit});
  steps.append(MotionStep{{0, ProbeSegment::ReturnHome}, M4d::Identity()});
  return steps;
}

// The same TCP pose with the other wrist configuration: A4 and A6 turned by 180 deg, A5 negated.
Kr10Kinematics::IkSolution otherWrist(Kr10Kinematics::IkSolution seed)
{
  seed.q[3] += seed.q[3] > 0.0 ? -180.0 : 180.0;
  seed.q[4] = -seed.q[4];
  seed.q[5] += seed.q[5] < 0.0 ? 180.0 : -180.0;
  seed.status = static_cast<uint8_t>(seed.status ^ (1u << 2)); // Kr10Kinematics wrist branch bit.
  return seed;
}

// Builds the timed points of one touch plan; borrows the plan and the worker-owned robot setup.
class TouchMotionBuilder
{
public:
  TouchMotionBuilder(const ProbeTouchPlan& plan, const ChamferRobotSetup& robot)
    : m_plan(plan), m_robot(robot), m_solver(robot) {}

  BuildResult build()
  {
    const V6d& home = m_robot.model.qHome;
    m_previous = m_solver.kinematics().configuration(home, m_solver.kinematics().solveFK(home));
    m_data.points.append(m_solver.forward(home, 0.0));
    m_data.minimumTransferClearance = std::numeric_limits<double>::infinity();
    m_data.minimumStrutClearance = std::numeric_limits<double>::infinity();
    for (const auto& [phase, target] : motionSteps(m_plan)) {
      const QString error = append(phase, target);
      if (error.isEmpty()) continue;
      // solveLinear already prefixes most of its errors with the phase text.
      const QString label = phase.text();
      return {{}, error.startsWith(label) ? error : label + QStringLiteral(": ") + error};
    }
    return {std::move(m_data), {}};
  }

private:
  QString append(const ProbeMotionPhase& phase, const M4d& target)
  {
    const qsizetype first = m_data.points.size() - 1;
    const QString error = ptpSegment(phase.segment) ? appendPtp(phase.segment, target) : appendLinear(phase, target);
    if (!error.isEmpty()) return error;
    ProbeMotionSegment segment{first, m_data.points.size() - 1, -1, phase};
    if (!ptpSegment(phase.segment) && !touchSegment(phase.segment)) {
      const auto spline = clampedJointSpline(m_data.points.mid(first, segment.last - first + 1), m_robot);
      if (!spline.error.isEmpty()) return spline.error;
      segment.spline = m_data.splines.size();
      m_data.splines.append(spline.coefficients);
    }
    m_data.segments.append(segment);
    return clearanceError(segment);
  }

  QString appendPtp(ProbeSegment segment, const M4d& target)
  {
    V6d end = m_robot.model.qHome;
    if (segment != ProbeSegment::ReturnHome) {
      const auto solved = m_solver.solveTcp(target, segment == ProbeSegment::Unwind ? otherWrist(m_previous) : m_previous);
      if (!solved.solution) return solved.error;
      m_previous = *solved.solution;
      end = m_previous.q;
    }
    const V6d start = m_data.points.back().joints;
    const double startTime = m_data.points.back().time;
    auto points = m_solver.samplePtp(start, end);
    if (points.size() > MaximumPoints - m_data.points.size())
      return QStringLiteral("Complete motion exceeds the 400000-point limit.");
    const double duration = ptpDuration(end - start, m_robot.timing);
    double lastTime = startTime;
    for (auto& point : points) {
      point.time = startTime + duration * inverseProgress(point.progress);
      if (!std::isfinite(point.time) || point.time <= lastTime)
        return QStringLiteral("Motion timing exceeds numerical resolution.");
      lastTime = point.time;
    }
    m_data.points.append(points);
    return {};
  }

  QString appendLinear(const ProbeMotionPhase& phase, const M4d& target)
  {
    const auto& timing = m_robot.timing;
    const LinearLimits limits = touchSegment(phase.segment) ? LinearLimits{ProbeApproachFeed, 0.0}
                                                            : LinearLimits{timing.transferFeed, timing.transferAcceleration};
    const auto profile = linearProfile(m_data.points.back().tcp, target, limits);
    if (!profile) return QStringLiteral("Invalid or degenerate LIN profile.");
    const auto schedule = linearSchedule(*profile, m_data.points.back().time);
    if (!schedule.error.isEmpty()) return schedule.error;
    if (schedule.localTimes.size() > MaximumPoints - m_data.points.size())
      return QStringLiteral("Complete motion exceeds the 400000-point limit.");
    const auto samples = m_solver.solveLinear(schedule, m_previous, phase.text());
    if (!samples.error.isEmpty()) return samples.error;
    m_data.points.append(samples.points);
    m_previous = samples.last;
    return {};
  }

  // Strut clearance of every new point; outside touches also the ball's distance outside the
  // contact shell, which chords of at most ProbeMaximumChordDegrees on the start shell keep.
  QString clearanceError(const ProbeMotionSegment& segment)
  {
    const auto& parameters = m_plan.parameters;
    const double contact = parameters.contactDistance();
    const double required = (contact + ProbeApproachGap) * std::cos(0.5 * ProbeMaximumChordDegrees * GeomConst::DegToRad)
        - contact - ClearanceTolerance;
    for (qsizetype i = segment.first + 1; i <= segment.last; ++i) {
      const auto& point = m_data.points[i];
      const V3d ball = point.tcp.block<3, 1>(0, 3);
      const double strut = parameters.strutClearance(ball);
      if (strut < ProbeApproachGap - ClearanceTolerance)
        return QStringLiteral("strut clearance %1 mm is below %2 mm at %3 s.")
            .arg(strut, 0, 'f', 4).arg(ProbeApproachGap).arg(point.time, 0, 'g', 12);
      m_data.minimumStrutClearance = std::min(m_data.minimumStrutClearance, strut);
      if (touchSegment(segment.phase.segment)) continue;
      const double gap = (ball - parameters.sphereCentre).norm() - contact;
      if (gap < required)
        return QStringLiteral("ball clearance to the sphere %1 mm is below %2 mm at %3 s.")
            .arg(gap, 0, 'f', 4).arg(required, 0, 'f', 4).arg(point.time, 0, 'g', 12);
      m_data.minimumTransferClearance = std::min(m_data.minimumTransferClearance, gap);
    }
    return {};
  }

  const ProbeTouchPlan& m_plan;
  const ChamferRobotSetup& m_robot;
  ChamferTcpSolver m_solver;
  ProbeTouchMotionData m_data;
  Kr10Kinematics::IkSolution m_previous;
};

} // namespace

QString ProbeMotionPhase::text() const
{
  static const std::array<const char*, 8> names{"to entry", "descend", "transfer", "approach", "retract",
                                                "wrist unwind", "retreat", "return HOME"};
  const QString name = QString::fromLatin1(names[static_cast<std::size_t>(segment)]);
  if (touch > 0) return QStringLiteral("Touch %1/%2: %3").arg(touch).arg(ProbeTouchCount).arg(name);
  const bool entry = segment == ProbeSegment::ToEntry || segment == ProbeSegment::Descend;
  return (entry ? QStringLiteral("Entry: ") : QStringLiteral("Exit: ")) + name;
}

ProbeTouchMotionResult ProbeTouchMotion::create(const ProbeTouchPlan& plan, const ChamferRobotSetup& robot)
{
  const auto rated = ratedRobotSetup(robot);
  if (!rated.robot) return {{}, rated.error};
  if (plan.touches.isEmpty()) return {{}, QStringLiteral("The touch plan has no touches.")};
  auto built = TouchMotionBuilder(plan, *rated.robot).build();
  if (!built.error.isEmpty()) return {{}, built.error};
  return {ProbeTouchMotion(*rated.robot, std::move(built.data)), {}};
}

std::optional<std::pair<V6d, ProbeMotionPhase>> ProbeTouchMotion::evaluateJoints(double seconds) const
{
  const auto& points = m_data.points;
  if (!std::isfinite(seconds) || m_data.segments.isEmpty()) return std::nullopt;
  const double time = std::clamp(seconds, 0.0, duration());
  auto segment = m_data.segments.cbegin();
  while (segment + 1 != m_data.segments.cend() && time > points[segment->last].time) ++segment;
  V6d joints = V6d::Zero();
  if (ptpSegment(segment->phase.segment)) {
    const auto& first = points[segment->first];
    const auto& last = points[segment->last];
    const double u = (time - first.time) / (last.time - first.time);
    joints = first.joints + smoothProgress(u) * (last.joints - first.joints);
  } else {
    qsizetype low = segment->first;
    qsizetype high = segment->last;
    while (low + 1 < high) {
      const auto middle = low + (high - low) / 2;
      if (points[middle].time <= time) low = middle;
      else high = middle;
    }
    const auto& a = points[low];
    const auto& b = points[high];
    const double fraction = (time - a.time) / (b.time - a.time);
    if (segment->spline < 0) joints = a.joints + fraction * (b.joints - a.joints);
    else {
      const auto& coefficients = m_data.splines[segment->spline + low - segment->first];
      joints = ((coefficients[3] * fraction + coefficients[2]) * fraction + coefficients[1]) * fraction
          + coefficients[0];
    }
  }
  if (!withinLimits(joints, m_robot.model)) return std::nullopt;
  return std::pair{joints, segment->phase};
}
