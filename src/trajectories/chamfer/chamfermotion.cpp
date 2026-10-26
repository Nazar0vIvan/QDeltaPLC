#include "trajectories/chamfermotion.h"
#include "chamfercentralvalidation.h"
#include "chamfertcpsolver.h"
#include "../motion/jointsegments.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

using namespace RoboCrap3D;

std::optional<LinearProfile> transferProfile(const ChamferPath& path, ChamferPhase side,
                                            const ChamferTimingParameters& timing)
{
  const bool entering = side == ChamferPhase::LeadIn;
  const auto staging = path.stagingPose(side);
  const auto endpoint = path.evaluatePhase(side, entering ? 0.0 : 1.0);
  if (!staging || !endpoint) return std::nullopt;
  return linearProfile(entering ? *staging : endpoint->tcp, entering ? endpoint->tcp : *staging,
                       {timing.transferFeed, timing.transferAcceleration});
}

QString centralPhaseName(ChamferMotionPhase phase)
{
  switch (phase) {
  case ChamferMotionPhase::TransferIn: return QStringLiteral("P_s to lead-in");
  case ChamferMotionPhase::LeadIn: return QStringLiteral("lead-in");
  case ChamferMotionPhase::Machining: return QStringLiteral("machining");
  case ChamferMotionPhase::LeadOut: return QStringLiteral("lead-out");
  case ChamferMotionPhase::TransferOut: return QStringLiteral("lead-out to P_s");
  default: return QStringLiteral("unknown phase");
  }
}

struct CompiledPoints
{
  QVector<ChamferJointPoint> points;
  std::array<qsizetype, 8> boundaries{};
  QString error;
  std::optional<ChamferCentralMotion> central;
};

class MotionCompiler
{
public:
  MotionCompiler(const ChamferPath& path, const ChamferRobotSetup& robot)
    : m_path(path), m_robot(robot), m_solver(robot), m_central(path, robot.timing) {}

  QVector<Kr10Kinematics::IkSolution> starts()
  {
    m_starts.clear();
    const auto target = m_path.stagingPose(ChamferPhase::LeadIn);
    if (!target) return {};
    // Probe valid arm/wrist seeds deterministically; the existing solver owns
    // branch selection. HOME is tried first, then interior limit fractions.
    appendStart(m_robot.model.qHome, *target);
    for (double shoulder : {0.15, 0.5, 0.85})
      for (double elbow : {0.15, 0.5, 0.85})
        for (double wrist : {0.25, 0.75}) {
          V6d seed = m_robot.model.qHome;
          seed[1] = seedAngle(1, shoulder);
          seed[2] = seedAngle(2, elbow);
          seed[4] = seedAngle(4, wrist);
          appendStart(seed, *target);
        }
    // A valid full revolution may need a different initial wrist winding.
    // Ask the solver to resolve those turns within limits, rather than wrapping
    // an already compiled path when a wrist reaches its travel limit.
    const QVector<Kr10Kinematics::IkSolution> branches = m_starts;
    for (const auto& branch : branches)
      for (double fourth : {0.0, 1.0})
        for (double sixth : {0.0, 1.0}) {
          V6d seed = branch.q;
          seed[3] = seedAngle(3, fourth);
          seed[5] = seedAngle(5, sixth);
          appendStart(seed, *target);
        }
    return m_starts;
  }

  CompiledPoints compile(const Kr10Kinematics::IkSolution& start)
  {
    m_output = {};
    m_output.points.append(m_solver.forward(m_robot.model.qHome, 0.0));
    m_phase = ChamferMotionPhase::Approach;
    if (!appendPtp(start.q)) return std::move(m_output);
    m_previous = start;
    m_output.boundaries[1] = m_output.points.size() - 1;
    m_phase = ChamferMotionPhase::TransferIn;
    m_output.error = appendTransfer(ChamferPhase::LeadIn);
    if (!m_output.error.isEmpty()) return std::move(m_output);
    m_output.boundaries[2] = m_output.points.size() - 1;
    auto validated = m_central.validate(m_robot, m_previous);
    if (!validated.data) { m_output.error = validated.error; return std::move(m_output); }
    const auto& central = m_central.motion();
    const auto& samples = validated.data->samples;
    if (samples.isEmpty() || (samples.front().solution.q - m_output.points.back().joints)
            .cwiseAbs().maxCoeff() > 1e-6) {
      m_output.error = QStringLiteral("Lead-in does not retain the transfer's joint configuration.");
      return std::move(m_output);
    }
    const double offset = m_output.points.back().time;
    std::size_t phase = 0;
    for (qsizetype i = 1; i < samples.size(); ++i) {
      if (m_output.points.size() >= 400000) {
        m_output.error = QStringLiteral("Complete motion exceeds the 400000-point limit.");
        return std::move(m_output);
      }
      const auto& sample = samples[i];
      ChamferJointPoint point{sample.solution.q, sample.actualTcp, sample.target.progress};
      point.time = offset + sample.time;
      if (phase >= 3) {
        m_output.error = QStringLiteral("Unexpected sample after the central endpoint.");
        return std::move(m_output);
      }
      const bool boundary = sample.time == central.phaseTimes()[phase + 1];
      if (boundary) point.progress = 1.0; // Shared point belongs to the ending phase.
      m_output.points.append(point);
      if (boundary) {
        m_output.boundaries[phase + 3] = m_output.points.size() - 1;
        ++phase;
      }
    }
    if (phase != 3) {
      m_output.error = QStringLiteral("Validated central motion is missing a phase endpoint.");
      return std::move(m_output);
    }
    m_previous = samples.back().solution;
    m_output.central = central; // Keep the shared preparation for any later candidate.
    m_phase = ChamferMotionPhase::TransferOut;
    m_output.error = appendTransfer(ChamferPhase::LeadOut);
    if (!m_output.error.isEmpty()) return std::move(m_output);
    m_output.boundaries[6] = m_output.points.size() - 1;
    m_phase = ChamferMotionPhase::ReturnHome;
    if (!appendPtp(m_robot.model.qHome)) return std::move(m_output);
    m_output.boundaries[7] = m_output.points.size() - 1;
    for (const auto& point : m_output.points)
      if (!rigid(point.tcp) || !withinLimits(point.joints, m_robot.model)) {
        m_output.error = QStringLiteral("Invalid FK pose or joint position in the assembled motion.");
        break;
      }
    return std::move(m_output);
  }

private:
  double seedAngle(std::size_t index, double fraction) const
  {
    const auto& joint = m_robot.model.joints[index];
    return (1.0 - fraction) * joint.qMin + fraction * joint.qMax;
  }

  void appendStart(const V6d& seed, const M4d& target)
  {
    const auto& kinematics = m_solver.kinematics();
    const auto previous = kinematics.configuration(seed, kinematics.solveFK(seed));
    const auto solution = m_solver.inverse(target, previous);
    if (!solution) return;
    for (const auto& existing : m_starts)
      if ((existing.q - solution->first.q).cwiseAbs().maxCoeff() < 1e-6) return;
    m_starts.append(solution->first);
  }

  bool appendPtp(const V6d& end)
  {
    const auto points = m_solver.samplePtp(m_output.points.back().joints, end);
    if (points.size() > 400000 - m_output.points.size()) {
      m_output.error = QStringLiteral("Complete motion exceeds the 400000-point limit.");
      return false;
    }
    m_output.points.append(points);
    return true;
  }

  QString appendTransfer(ChamferPhase side)
  {
    const auto profile = transferProfile(m_path, side, m_robot.timing);
    if (!profile) return QStringLiteral("Invalid or degenerate staging transfer profile.");
    const M4d& actualStart = m_output.points.back().tcp;
    if ((actualStart.block<3, 1>(0, 3) - profile->first.block<3, 1>(0, 3)).norm() > Kr10PosEps
        || (actualStart.block<3, 3>(0, 0) - profile->first.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
      return QStringLiteral("Staging transfer does not match its preceding TCP endpoint.");
    const auto schedule = linearSchedule(*profile, m_output.points.back().time);
    if (!schedule.error.isEmpty()) return schedule.error;
    if (schedule.localTimes.size() > 400000 - m_output.points.size())
      return QStringLiteral("Complete motion exceeds the 400000-point limit.");
    const auto samples = m_solver.solveLinear(schedule, m_previous, centralPhaseName(m_phase));
    if (!samples.error.isEmpty()) return samples.error;
    m_output.points.append(samples.points);
    m_previous = samples.last;
    return {};
  }

  const ChamferPath& m_path;
  const ChamferRobotSetup& m_robot;
  ChamferTcpSolver m_solver;
  ChamferCentralPreparation m_central;
  ChamferMotionPhase m_phase = ChamferMotionPhase::TransferIn;
  CompiledPoints m_output;
  QVector<Kr10Kinematics::IkSolution> m_starts;
  Kr10Kinematics::IkSolution m_previous;
};

class HomeTimer
{
public:
  HomeTimer(CompiledPoints data, const ChamferRobotSetup& robot)
    : m_data(std::move(data)), m_robot(robot) {}

  CompiledPoints run()
  {
    timePtp(0);
    const double offset = m_data.points[m_data.boundaries[1]].time;
    for (qsizetype i = m_data.boundaries[1] + 1; i <= m_data.boundaries[6]; ++i)
      m_data.points[i].time += offset;
    timePtp(6);
    for (qsizetype i = 1; i < m_data.points.size(); ++i)
      if (!std::isfinite(m_data.points[i].time)
          || m_data.points[i].time <= m_data.points[i - 1].time) {
        m_data.error = QStringLiteral("Motion timing exceeds numerical resolution.");
        break;
      }
    return std::move(m_data);
  }

private:
  void timePtp(std::size_t phase)
  {
    const auto first = m_data.boundaries[phase];
    const auto last = m_data.boundaries[phase + 1];
    const V6d delta = m_data.points[last].joints - m_data.points[first].joints;
    const double duration = ptpDuration(delta, m_robot.timing);
    const double startTime = m_data.points[first].time;
    for (qsizetype i = first + 1; i <= last; ++i) {
      const double u = inverseProgress(m_data.points[i].progress);
      m_data.points[i].time = startTime + duration * u;
      const V6d velocity = delta * (smoothVelocity(u) / duration);
      if (!velocity.allFinite())
        m_data.error = QStringLiteral("Motion timing exceeds numerical resolution.");
    }
  }

  CompiledPoints m_data;
  const ChamferRobotSetup& m_robot;
};

} // namespace

ChamferMotion::ChamferMotion(const ChamferPath& path, const ChamferRobotSetup& robot,
                             QVector<ChamferJointPoint> points, const std::array<qsizetype, 8>& boundaries)
  : m_parameters(path.parameters()), m_robot(robot), m_points(std::move(points)), m_boundaries(boundaries) {}

void ChamferMotion::setCentralMotion(ChamferCentralMotion motion)
{
  m_centralMotion = std::move(motion);
}

QString ChamferMotion::preparePlaybackSplines()
{
  m_jointSpline.reserve(m_boundaries[6] - m_boundaries[1]);
  const std::array<std::array<std::size_t, 2>, 3> ranges{{{1, 2}, {2, 5}, {5, 6}}};
  const std::array<QString, 3> names{QStringLiteral("P_s to lead-in"),
      QStringLiteral("Lead-in / machining / lead-out"), QStringLiteral("lead-out to P_s")};
  for (std::size_t index = 0; index < ranges.size(); ++index) {
    const auto first = m_boundaries[ranges[index][0]];
    const auto last = m_boundaries[ranges[index][1]];
    if (first < 0 || last >= m_points.size() || last <= first)
      return QStringLiteral("%1: Missing joint spline points.").arg(names[index]);
    // Consecutive ranges append the intervals indexed from boundaries[1].
    const auto spline = clampedJointSpline(m_points.mid(first, last - first + 1), m_robot);
    if (!spline.error.isEmpty()) return QStringLiteral("%1: %2").arg(names[index], spline.error);
    m_jointSpline.append(spline.coefficients);
  }
  return {};
}

ChamferMotionResult ChamferMotion::create(const ChamferPath& path, const ChamferRobotSetup& robot)
{
  const auto rated = ratedRobotSetup(robot);
  if (!rated.robot) return {{}, rated.error};
  const ChamferRobotSetup& boundedRobot = *rated.robot;
  MotionCompiler compiler(path, boundedRobot);
  QString error = QStringLiteral("No IK solution for the approach P_s pose in the searched configurations.");
  for (const auto& start : compiler.starts()) {
    auto compiled = compiler.compile(start);
    if (!compiled.error.isEmpty()) { error = compiled.error; continue; }
    auto timed = HomeTimer(std::move(compiled), boundedRobot).run();
    if (!timed.error.isEmpty()) { error = timed.error; continue; }
    ChamferMotion motion(path, boundedRobot, std::move(timed.points), timed.boundaries);
    if (!timed.central) return {{}, QStringLiteral("Missing prescribed central trajectory.")};
    motion.setCentralMotion(std::move(*timed.central));
    const QString splineError = motion.preparePlaybackSplines();
    if (!splineError.isEmpty()) { error = splineError; continue; }
    return {std::move(motion), {}};
  }
  return {{}, error};
}

std::optional<std::pair<V6d, ChamferMotionPhase>> ChamferMotion::evaluateJoints(double seconds) const
{
  if (!std::isfinite(seconds) || m_points.isEmpty()) return std::nullopt;
  const double time = std::clamp(seconds, 0.0, duration());
  if (time == 0.0) return std::pair{m_points.front().joints, ChamferMotionPhase::Approach};
  if (time == duration()) return std::pair{m_points.back().joints, ChamferMotionPhase::ReturnHome};
  std::size_t phase = 0;
  while (phase < 6 && time >= m_points[m_boundaries[phase + 1]].time) ++phase;
  const auto first = m_boundaries[phase];
  const auto last = m_boundaries[phase + 1];
  V6d joints = V6d::Zero();
  if (phase == 0 || phase == 6) {
    const double u = (time - m_points[first].time) / (m_points[last].time - m_points[first].time);
    joints = m_points[first].joints + smoothProgress(u) * (m_points[last].joints - m_points[first].joints);
  } else {
    qsizetype low = first;
    qsizetype high = last;
    while (low + 1 < high) {
      const auto middle = low + (high - low) / 2;
      if (m_points[middle].time <= time) low = middle;
      else high = middle;
    }
    const auto& a = m_points[low];
    const auto& b = m_points[high];
    const double fraction = (time - a.time) / (b.time - a.time);
    const qsizetype interval = low - m_boundaries[1];
    if (interval < 0 || interval >= m_jointSpline.size()) return std::nullopt;
    const auto& coefficients = m_jointSpline[interval];
    // Preserve exact IK values at saved knots, including full-turn phase boundaries.
    if (time == a.time) joints = a.joints;
    else if (time == b.time) joints = b.joints;
    else joints = ((coefficients[3] * fraction + coefficients[2]) * fraction
                   + coefficients[1]) * fraction + coefficients[0];
  }
  if (!withinLimits(joints, m_robot.model)) return std::nullopt;
  return std::pair{joints, static_cast<ChamferMotionPhase>(phase)};
}
