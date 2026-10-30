#include "trajectories/probetouchmotion.h"

#include "../motion/motionbuilder.h"

#include <algorithm>
#include <limits>
#include <utility>

using namespace RoboCrap3D;

namespace {

constexpr double ClearanceTolerance = 0.0001; // mm

struct MotionStep
{
  ProbeMotionPhase phase;
  M4d target; // TCP pose at the end of the segment; unused by ReturnHome.
};

struct BuildResult
{
  std::optional<TimedMotion> motion;
  ProbeClearances clearances;
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
    if (number > 1) steps.append(MotionStep{{number, ProbeSegment::Transfer}, touch.start});
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
    : m_plan(plan), m_robot(robot), m_motion(robot) {}

  BuildResult build()
  {
    m_clearances = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    for (const auto& [phase, target] : motionSteps(m_plan)) {
      const qsizetype first = m_motion.points().size() - 1;
      QString error = append(phase, target);
      if (error.isEmpty()) error = clearanceError(phase, first);
      if (error.isEmpty()) continue;
      // MotionBuilder errors already start with the phase text.
      const QString label = phase.text();
      return {std::nullopt, {}, error.startsWith(label) ? error : label + QStringLiteral(": ") + error};
    }
    return {m_motion.finish(), m_clearances, {}};
  }

private:
  QString append(const ProbeMotionPhase& phase, const M4d& target)
  {
    const QString label = phase.text();
    if (phase.segment == ProbeSegment::ReturnHome) return m_motion.appendPtp(m_motion.home(), label);
    if (ptpSegment(phase.segment)) {
      const auto& seed = m_motion.seed();
      const auto solved = m_motion.solver().solveTcp(target, phase.segment == ProbeSegment::Unwind ? otherWrist(seed) : seed);
      if (!solved.solution) return solved.error;
      return m_motion.appendPtp(*solved.solution, label);
    }
    const auto& timing = m_robot.timing;
    const bool touching = touchSegment(phase.segment);
    const LinearLimits limits = touching ? LinearLimits{ProbeApproachFeed, 0.0}
                                         : LinearLimits{timing.transferFeed, timing.transferAcceleration};
    auto profile = linearProfile(m_motion.points().back().tcp, target, limits);
    if (profile && phase.segment == ProbeSegment::Transfer)
      profile = circularProfile(*profile, m_plan.parameters.sphereCentre);
    if (!profile) return QStringLiteral("Invalid or degenerate LIN or CIRC profile.");
    return m_motion.appendPath(LinearPath(*profile, label),
                               touching ? MotionInterpolation::JointLinear : MotionInterpolation::Spline);
  }

  // Strut clearance of every point after first; outside touches also the ball's distance outside
  // the contact shell.
  QString clearanceError(const ProbeMotionPhase& phase, qsizetype first)
  {
    const auto& parameters = m_plan.parameters;
    const auto& points = m_motion.points();
    const double contact = parameters.contactDistance();
    const double required = ProbeApproachGap - ClearanceTolerance;
    for (qsizetype i = first + 1; i < points.size(); ++i) {
      const auto& point = points[i];
      const V3d ball = point.tcp.block<3, 1>(0, 3);
      const double strut = parameters.strutClearance(ball);
      if (strut < ProbeApproachGap - ClearanceTolerance)
        return QStringLiteral("strut clearance %1 mm is below %2 mm at %3 s.")
            .arg(strut, 0, 'f', 4).arg(ProbeApproachGap).arg(point.time, 0, 'g', 12);
      m_clearances.strut = std::min(m_clearances.strut, strut);
      if (touchSegment(phase.segment)) continue;
      const double gap = (ball - parameters.sphereCentre).norm() - contact;
      if (gap < required)
        return QStringLiteral("ball clearance to the sphere %1 mm is below %2 mm at %3 s.")
            .arg(gap, 0, 'f', 4).arg(required, 0, 'f', 4).arg(point.time, 0, 'g', 12);
      m_clearances.transfer = std::min(m_clearances.transfer, gap);
    }
    return {};
  }

  const ProbeTouchPlan& m_plan;
  const ChamferRobotSetup& m_robot;
  MotionBuilder m_motion;
  ProbeClearances m_clearances;
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
  return {ProbeTouchMotion(std::move(*built.motion), built.clearances), {}};
}
