#include "trajectories/probetouchmotion.h"

#include "../motion/motionbuilder.h"

#include <algorithm>
#include <limits>
#include <utility>

using namespace RoboCrap3D;

namespace {

constexpr double ClearanceTolerance = 0.001; // mm; covers the 1e-4 mm FK reconstruction of stored poses.

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
  return segment == ProbeSegment::ToEntry || segment == ProbeSegment::ReturnHome;
}

// Segments inside the start shell, exempt from the sphere clearance check.
bool touchSegment(ProbeSegment segment)
{
  return segment == ProbeSegment::Approach || segment == ProbeSegment::Retract;
}

// A pass in another probe orientation is reached through the previous exit, a PTP to its entry and a
// descent; a pass in the same orientation by a transfer, omitted when it starts where the previous
// pass ended (a zero-length CIRC is degenerate).
QVector<MotionStep> motionSteps(const ProbeTouchPlan& plan)
{
  QVector<MotionStep> steps;
  for (int index = 0; index < plan.passes.size(); ++index) {
    const ProbeTouchPass& pass = plan.passes[index];
    const M4d& first = pass.touches.front().start;
    if (index == 0 || pass.group != plan.passes[index - 1].group) {
      if (index > 0) steps.append(MotionStep{{index - 1, 0, ProbeSegment::Retreat}, plan.passes[index - 1].exit});
      steps.append(MotionStep{{index, 0, ProbeSegment::ToEntry}, pass.entry});
      steps.append(MotionStep{{index, 0, ProbeSegment::Descend}, first});
    } else if (first != plan.passes[index - 1].touches.back().start) {
      steps.append(MotionStep{{index, 1, ProbeSegment::Transfer}, first});
    }
    int number = 0;
    for (const ProbeTouch& touch : pass.touches) {
      ++number;
      if (number > 1) steps.append(MotionStep{{index, number, ProbeSegment::Transfer}, touch.start});
      steps.append(MotionStep{{index, number, ProbeSegment::Approach}, touch.contact});
      steps.append(MotionStep{{index, number, ProbeSegment::Retract}, touch.start});
    }
  }
  steps.append(MotionStep{{static_cast<int>(plan.passes.size()) - 1, 0, ProbeSegment::Retreat}, plan.passes.back().exit});
  steps.append(MotionStep{{-1, 0, ProbeSegment::ReturnHome}, M4d::Identity()});
  return steps;
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
      const QString label = phase.text(m_plan);
      return {std::nullopt, {}, error.startsWith(label) ? error : label + QStringLiteral(": ") + error};
    }
    return {m_motion.finish(), m_clearances, {}};
  }

private:
  QString append(const ProbeMotionPhase& phase, const M4d& target)
  {
    const QString label = phase.text(m_plan);
    if (phase.segment == ProbeSegment::ReturnHome) return m_motion.appendPtp(m_motion.home(), label);
    if (ptpSegment(phase.segment)) {
      const auto solved = m_motion.solver().solveTcp(target, m_motion.seed());
      if (!solved.solution) return solved.error;
      return m_motion.appendPtp(*solved.solution, label);
    }
    const auto& timing = m_robot.timing;
    // Only the Approach measures: constant search feed without ramps, interpolated joint-linearly in
    // playback. Retract and the other LIN/CIRC moves use the transfer limits.
    const bool searching = phase.segment == ProbeSegment::Approach;
    const LinearLimits limits = searching ? LinearLimits{m_plan.parameters.searchFeed, 0.0}
                                          : LinearLimits{timing.transferFeed, timing.transferAcceleration};
    auto profile = linearProfile(m_motion.points().back().tcp, target, limits);
    // Transfers are CIRC arcs on the start shell around the sphere centre.
    if (profile && phase.segment == ProbeSegment::Transfer)
      profile = circularProfile(*profile, m_plan.parameters.sphereCentre);
    if (!profile) return QStringLiteral("Invalid or degenerate LIN or CIRC profile.");
    return m_motion.appendPath(LinearPath(*profile, label),
                               searching ? MotionInterpolation::JointLinear : MotionInterpolation::Spline);
  }

  // Strut clearance of every point after first; outside touches also the ball's distance outside
  // the contact shell, which the CIRC transfers on the start shell keep at ProbeApproachGap.
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

QString ProbeMotionPhase::text(const ProbeTouchPlan& plan) const
{
  static const std::array<const char*, 7> names{"to entry", "descend", "transfer", "approach", "retract",
                                                "retreat", "return HOME"};
  const QString name = QString::fromLatin1(names[static_cast<std::size_t>(segment)]);
  if (pass < 0) return QStringLiteral("Exit: ") + name;
  const ProbeTouchPass& owner = plan.passes[pass];
  if (touch > 0)
    return QStringLiteral("%1, touch %2/%3: %4").arg(owner.label()).arg(touch).arg(owner.touches.size()).arg(name);
  return owner.label() + QStringLiteral(": ") + name;
}

ProbeTouchMotionResult ProbeTouchMotion::create(const ProbeTouchPlan& plan, const ChamferRobotSetup& robot)
{
  const auto rated = ratedRobotSetup(robot);
  if (!rated.robot) return {{}, rated.error};
  if (plan.passes.isEmpty()) return {{}, QStringLiteral("The touch plan has no passes.")};
  for (const ProbeTouchPass& pass : plan.passes)
    if (pass.touches.isEmpty()) return {{}, pass.label() + QStringLiteral(": the pass has no touches.")};
  auto built = TouchMotionBuilder(plan, *rated.robot).build();
  if (!built.error.isEmpty()) return {{}, built.error};
  return {ProbeTouchMotion(std::move(*built.motion), plan, built.clearances), {}};
}
