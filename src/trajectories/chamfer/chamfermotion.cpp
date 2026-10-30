#include "trajectories/chamfermotion.h"
#include "chamfercentralvalidation.h"
#include "../motion/motionbuilder.h"

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

struct CompiledMotion
{
  std::optional<TimedMotion> motion;
  std::array<qsizetype, 8> boundaries{};
  QString error;
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

  // PTP HOME to P_s, LIN P_s to the lead-in start, the central curve (one spline over its three
  // phases), LIN lead-out end to P_s and PTP P_s to HOME.
  CompiledMotion compile(const Kr10Kinematics::IkSolution& start)
  {
    auto& motion = m_motion.emplace(m_robot);
    QString error = motion.appendPtp(start, QStringLiteral("HOME to P_s"));
    if (error.isEmpty()) error = appendTransfer(ChamferPhase::LeadIn, QStringLiteral("P_s to lead-in"));
    if (error.isEmpty()) error = appendCentral();
    if (error.isEmpty()) error = appendTransfer(ChamferPhase::LeadOut, QStringLiteral("Lead-out to P_s"));
    if (error.isEmpty()) error = motion.appendPtp(motion.home(), QStringLiteral("Return HOME"));
    if (!error.isEmpty()) return {{}, {}, error};
    for (const auto& point : motion.points())
      if (!rigid(point.tcp) || !withinLimits(point.joints, m_robot.model))
        return {{}, {}, QStringLiteral("Invalid FK pose or joint position in the assembled motion.")};
    auto timed = motion.finish();
    if (timed.phases().size() != 7) return {{}, {}, QStringLiteral("Assembled motion lacks a phase.")};
    std::array<qsizetype, 8> boundaries{};
    for (std::size_t phase = 0; phase < 7; ++phase)
      boundaries[phase + 1] = timed.phases()[static_cast<qsizetype>(phase)].last;
    return {std::move(timed), boundaries, {}};
  }

  // Shared by every candidate after a successful compile.
  const ChamferCentralMotion& centralMotion() const { return m_central.motion(); }

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

  QString appendTransfer(ChamferPhase side, const QString& label)
  {
    const auto profile = transferProfile(m_path, side, m_robot.timing);
    if (!profile) return label + QStringLiteral(": Invalid or degenerate staging transfer profile.");
    return m_motion->appendPath(LinearPath(*profile, label), MotionInterpolation::Spline);
  }

  QString appendCentral()
  {
    const QString seedError = chamferJointPositionError(m_motion->seed().q, m_robot.model);
    if (!seedError.isEmpty()) return QStringLiteral("Invalid initial IK seed: %1").arg(seedError);
    const QString error = m_central.prepare();
    if (!error.isEmpty()) return error;
    return m_motion->appendPath(CentralPath(m_central), MotionInterpolation::Spline);
  }

  const ChamferPath& m_path;
  const ChamferRobotSetup& m_robot;
  ChamferTcpSolver m_solver;
  ChamferCentralPreparation m_central;
  QVector<Kr10Kinematics::IkSolution> m_starts;
  std::optional<MotionBuilder> m_motion; // The candidate being compiled.
};

} // namespace

ChamferMotion::ChamferMotion(const ChamferPath& path, TimedMotion timed, const std::array<qsizetype, 8>& boundaries)
  : m_parameters(path.parameters()), m_timed(std::move(timed)), m_boundaries(boundaries) {}

void ChamferMotion::setCentralMotion(ChamferCentralMotion motion)
{
  m_centralMotion = std::move(motion);
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
    if (!compiled.motion) { error = compiled.error; continue; }
    ChamferMotion motion(path, std::move(*compiled.motion), compiled.boundaries);
    motion.setCentralMotion(compiler.centralMotion());
    return {std::move(motion), {}};
  }
  return {{}, error};
}
