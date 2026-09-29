#include "chamfermotion.h"

#include "3d/robot/kinematics/kr10kinematics.h"
#include "geometry/utils.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

using namespace RoboCrap3D;

bool rigid(const M4d& transform)
{
  return transform.allFinite()
      && isBasis(transform.block<3, 1>(0, 0), transform.block<3, 1>(0, 1),
                 transform.block<3, 1>(0, 2), Kr10RotEps)
      && (transform.row(3) - Eigen::RowVector4d(0.0, 0.0, 0.0, 1.0)).cwiseAbs().maxCoeff()
          <= GeomConst::Eps;
}

bool withinLimits(const V6d& joints, const Kr10KinematicModel& model)
{
  if (!joints.allFinite()) return false;
  if (IgnorePreviewJointPositionLimits) return true;
  for (std::size_t i = 0; i < DofCount; ++i)
    if (joints[i] < model.joints[i].qMin || joints[i] > model.joints[i].qMax) return false;
  return true;
}

bool validRobot(const ChamferRobotSetup& robot)
{
  const auto& model = robot.model;
  if (!rigid(robot.flangeToTcp) || !model.qHome.allFinite()) return false;
  const auto& ik = model.ik;
  if (!std::isfinite(ik.sx) || !std::isfinite(ik.sz) || !std::isfinite(ik.a)
      || !std::isfinite(ik.bx) || !std::isfinite(ik.by) || !std::isfinite(ik.dF)) return false;
  for (const auto& joint : model.joints) {
    // The existing analytic solver divides by axis.z and resolves integer turns.
    if (!rigid(joint.localTransform) || !joint.axis.allFinite()
        || std::abs(std::abs(joint.axis.z()) - 1.0) > GeomConst::Eps
        || std::abs(joint.axis.x()) > GeomConst::Eps || std::abs(joint.axis.y()) > GeomConst::Eps
        || !std::isfinite(joint.qMin) || !std::isfinite(joint.qMax)
        || joint.qMin > joint.qMax || std::abs(joint.qMin) > 1e6 || std::abs(joint.qMax) > 1e6)
      return false;
  }
  return withinLimits(model.qHome, model) && Kr10Kinematics(model).isValid();
}

// Central phase target shared by IK compilation and timed interpolation checks.
std::optional<M4d> centralTarget(const ChamferPath& path, ChamferMotionPhase phase, double progress)
{
  if (!std::isfinite(progress) || progress < 0.0 || progress > 1.0) return std::nullopt;
  if (phase == ChamferMotionPhase::TransferIn || phase == ChamferMotionPhase::TransferOut) {
    const bool entering = phase == ChamferMotionPhase::TransferIn;
    const auto lead = entering ? ChamferPhase::LeadIn : ChamferPhase::LeadOut;
    const auto staging = path.stagingPose(lead);
    const auto endpoint = path.evaluatePhase(lead, entering ? 0.0 : 1.0);
    if (!staging || !endpoint) return std::nullopt;
    const M4d first = entering ? *staging : endpoint->tcp;
    const M4d last = entering ? endpoint->tcp : *staging;
    if (progress == 0.0) return first;
    if (progress == 1.0) return last;
    M4d target = first;
    target.block<3, 1>(0, 3) = (1.0 - progress) * first.block<3, 1>(0, 3)
        + progress * last.block<3, 1>(0, 3);
    return target;
  }
  ChamferPhase curve;
  switch (phase) {
  case ChamferMotionPhase::LeadIn: curve = ChamferPhase::LeadIn; break;
  case ChamferMotionPhase::Machining: curve = ChamferPhase::Machining; break;
  case ChamferMotionPhase::LeadOut: curve = ChamferPhase::LeadOut; break;
  default: return std::nullopt;
  }
  const auto sample = path.evaluatePhase(curve, progress);
  if (!sample) return std::nullopt;
  return sample->tcp;
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
  double centralTimeScale = 1.0;
  bool needsRefinement = false;
};

class MotionCompiler
{
public:
  MotionCompiler(const ChamferPath& path, const ChamferRobotSetup& robot)
    : m_path(path), m_robot(robot), m_solver(robot.model), m_tcpToFlange(inverseRigidTransform(robot.flangeToTcp)) {}

  QVector<V6d> starts()
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
    const QVector<V6d> branches = m_starts;
    for (const auto& branch : branches)
      for (double fourth : {0.0, 1.0})
        for (double sixth : {0.0, 1.0}) {
          V6d seed = branch;
          seed[3] = seedAngle(3, fourth);
          seed[5] = seedAngle(5, sixth);
          appendStart(seed, *target);
        }
    return m_starts;
  }

  CompiledPoints compile(const V6d& start, int refinement)
  {
    m_refinement = refinement;
    m_output = {};
    m_output.points.append(forward(m_robot.model.qHome, 0.0));
    appendPtp(start);
    m_output.boundaries[1] = m_output.points.size() - 1;
    const auto sampled = m_path.sample();
    if (!sampled.path) { m_output.error = sampled.error; return std::move(m_output); }
    for (std::size_t phase = 1; phase <= 5; ++phase) {
      m_phase = static_cast<ChamferMotionPhase>(phase);
      QVector<double> progressValues;
      if (phase == 1 || phase == 5) {
        // Multiple intervals permit acceleration from/to rest at transfer corners.
        for (int i = 1; i <= 16; ++i) progressValues.append(i / 16.0);
      } else {
        const auto curve = phase - 2;
        for (qsizetype j = sampled.path->boundaries[curve] + 1;
             j <= sampled.path->boundaries[curve + 1]; ++j)
          progressValues.append(sampled.path->points[j].progress);
      }
      double previous = 0.0;
      for (double progress : progressValues) {
        if (!appendCurve(previous, progress, 0)) {
          m_output.error = QStringLiteral("No continuous IK path in %1 at %2%: unreachable, singular, joint limit or refinement limit.")
              .arg(centralPhaseName(m_phase)).arg(100.0 * progress, 0, 'f', 2);
          return std::move(m_output);
        }
        previous = progress;
      }
      m_output.boundaries[phase + 1] = m_output.points.size() - 1;
    }
    appendPtp(m_robot.model.qHome);
    m_output.boundaries[7] = m_output.points.size() - 1;
    for (const auto& point : m_output.points)
      if (!rigid(point.tcp) || !withinLimits(point.joints, m_robot.model)) {
        m_output.error = QStringLiteral("Invalid forward-kinematic pose in the compiled motion.");
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

  std::optional<V6d> inverse(const M4d& tcp, const V6d& seed) const
  {
    const M4d flange = tcp * m_tcpToFlange;
    const auto solution = m_solver.solveIK(flange, seed);
    if (!solution || !withinLimits(solution->q, m_robot.model)) return std::nullopt;
    const M4d actual = m_solver.solveFK(solution->q).back();
    const M4d error = inverseRigidTransform(flange) * actual;
    if (!error.allFinite() || error.block<3, 1>(0, 3).norm() > Kr10PosEps
        || (error.block<3, 3>(0, 0) - M3d::Identity()).cwiseAbs().maxCoeff() > Kr10RotEps)
      return std::nullopt;
    return solution->q;
  }

  void appendStart(const V6d& seed, const M4d& target)
  {
    const auto solution = inverse(target, seed);
    if (!solution) return;
    for (const auto& existing : m_starts)
      if ((existing - *solution).cwiseAbs().maxCoeff() < 1e-6) return;
    m_starts.append(*solution);
  }

  ChamferJointPoint forward(const V6d& joints, double progress) const
  {
    return {joints, m_solver.solveFK(joints).back() * m_robot.flangeToTcp, progress};
  }

  void appendPtp(const V6d& end)
  {
    const V6d start = m_output.points.back().joints;
    const int intervals = std::max(1, static_cast<int>(std::ceil((end - start).cwiseAbs().maxCoeff() / 2.0)));
    for (int i = 1; i <= intervals; ++i) {
      const double progress = static_cast<double>(i) / intervals;
      const V6d joints = i == intervals ? end : V6d(start + progress * (end - start));
      m_output.points.append(forward(joints, progress));
    }
  }

  // Progress interval and depth define bounded continuous-IK refinement.
  bool appendCurve(double from, double to, int depth)
  {
    if (m_output.points.size() >= 200000) return false;
    const V6d start = m_output.points.back().joints;
    const auto target = centralTarget(m_path, m_phase, to);
    if (!target) return false;
    const auto end = inverse(*target, start);
    if (depth >= m_refinement && end && (*end - start).cwiseAbs().maxCoeff() <= 5.0) {
      m_output.points.append(forward(*end, to));
      return true;
    }
    if (depth >= 12) return false;
    const double middle = 0.5 * (from + to);
    return appendCurve(from, middle, depth + 1) && appendCurve(middle, to, depth + 1);
  }

  const ChamferPath& m_path;
  const ChamferRobotSetup& m_robot;
  Kr10Kinematics m_solver;
  M4d m_tcpToFlange = M4d::Identity();
  ChamferMotionPhase m_phase = ChamferMotionPhase::TransferIn;
  CompiledPoints m_output;
  QVector<V6d> m_starts;
  int m_refinement = 0;
};

double smoothProgress(double u)
{
  return u * u * u * (10.0 + u * (-15.0 + 6.0 * u));
}

double smoothVelocity(double u)
{
  return 30.0 * u * u * (1.0 - u) * (1.0 - u);
}

double inverseProgress(double progress)
{
  if (progress <= 0.0) return 0.0;
  if (progress >= 1.0) return 1.0;
  double low = 0.0;
  double high = 1.0;
  for (int i = 0; i < 50; ++i) {
    const double middle = 0.5 * (low + high);
    if (smoothProgress(middle) < progress) low = middle;
    else high = middle;
  }
  return 0.5 * (low + high);
}

V6d interpolate(const ChamferJointPoint& first, const ChamferJointPoint& last, double u)
{
  const double h = last.time - first.time;
  const double u2 = u * u;
  const double u3 = u2 * u;
  return (2.0 * u3 - 3.0 * u2 + 1.0) * first.joints
      + (u3 - 2.0 * u2 + u) * h * first.velocity
      + (-2.0 * u3 + 3.0 * u2) * last.joints
      + (u3 - u2) * h * last.velocity;
}

bool validTiming(const ChamferTimingParameters& timing)
{
  return std::isfinite(timing.leadInFeed) && timing.leadInFeed > 0.0
      && std::isfinite(timing.machiningFeed) && timing.machiningFeed > 0.0
      && std::isfinite(timing.leadOutFeed) && timing.leadOutFeed > 0.0
      && std::isfinite(timing.cartesianAcceleration) && timing.cartesianAcceleration > 0.0
      && timing.jointSpeed.allFinite() && timing.jointSpeed.minCoeff() > 0.0
      && timing.jointAcceleration.allFinite() && timing.jointAcceleration.minCoeff() > 0.0
      && std::isfinite(timing.auxiliaryScale) && timing.auxiliaryScale > 0.0
      && timing.auxiliaryScale <= 1.0;
}

class MotionTimer
{
public:
  // Complete geometric motion, curve evaluator and robot limits define timing.
  MotionTimer(CompiledPoints data, const ChamferPath& path, const ChamferRobotSetup& robot)
    : m_data(std::move(data)), m_path(path), m_robot(robot), m_solver(robot.model) {}

  CompiledPoints run()
  {
    timePtp(0);
    if (!timeCentral()) {
      if (m_data.error.isEmpty()) m_data.error = QStringLiteral("Degenerate or non-finite motion timing.");
      return std::move(m_data);
    }
    timePtp(6);
    for (qsizetype i = 1; i < m_data.points.size(); ++i)
      if (!std::isfinite(m_data.points[i].time)
          || m_data.points[i].time <= m_data.points[i - 1].time
          || !m_data.points[i].velocity.allFinite()) {
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
    const auto& settings = m_robot.timing;
    // Quintic synchronized progress: max s'=1.875, max |s''|=10/sqrt(3).
    double duration = 0.001; // A coincident HOME endpoint retains a finite phase.
    for (int joint = 0; joint < 6; ++joint) {
      duration = std::max(duration, 1.875 * std::abs(delta[joint])
          / (settings.jointSpeed[joint] * settings.auxiliaryScale));
      duration = std::max(duration, std::sqrt((10.0 / std::sqrt(3.0)) * std::abs(delta[joint])
          / (settings.jointAcceleration[joint] * settings.auxiliaryScale)));
    }
    const double startTime = m_data.points[first].time;
    m_data.points[first].velocity.setZero();
    for (qsizetype i = first + 1; i <= last; ++i) {
      const double u = inverseProgress(m_data.points[i].progress);
      m_data.points[i].time = startTime + duration * u;
      m_data.points[i].velocity = delta * (smoothVelocity(u) / duration);
    }
  }

  double feed(qsizetype interval) const
  {
    if (interval < m_data.boundaries[3]) return m_robot.timing.leadInFeed;
    if (interval < m_data.boundaries[4]) return m_robot.timing.machiningFeed;
    return m_robot.timing.leadOutFeed;
  }

  void setVelocities()
  {
    const auto first = m_data.boundaries[1];
    const auto last = m_data.boundaries[6];
    m_data.points[first].velocity.setZero();
    m_data.points[last].velocity.setZero();
    for (qsizetype i = first + 1; i < last; ++i) {
      auto& point = m_data.points[i];
      if (i == m_data.boundaries[2] || i == m_data.boundaries[5]) {
        point.velocity.setZero();
        continue;
      }
      const auto& before = m_data.points[i - 1];
      const auto& after = m_data.points[i + 1];
      const double left = point.time - before.time;
      const double right = after.time - point.time;
      for (int joint = 0; joint < 6; ++joint) {
        const double a = (point.joints[joint] - before.joints[joint]) / left;
        const double b = (after.joints[joint] - point.joints[joint]) / right;
        point.velocity[joint] = 0.0;
        // Monotone cubic slopes prevent overshooting joint-position limits.
        if ((a > 0.0 && b > 0.0) || (a < 0.0 && b < 0.0)) {
          const double w1 = 2.0 * right + left;
          const double w2 = right + 2.0 * left;
          point.velocity[joint] = (w1 + w2) / (w1 / a + w2 / b);
        }
      }
    }
  }

  double jointScale(qsizetype interval) const
  {
    const auto& a = m_data.points[interval];
    const auto& b = m_data.points[interval + 1];
    const double h = b.time - a.time;
    const V6d slope = (b.joints - a.joints) / h;
    const V6d middle = 3.0 * slope - a.velocity - b.velocity;
    const V6d acceleration0 = (6.0 * slope - 4.0 * a.velocity - 2.0 * b.velocity) / h;
    const V6d acceleration1 = (-6.0 * slope + 2.0 * a.velocity + 4.0 * b.velocity) / h;
    double scale = 1.0;
    for (int joint = 0; joint < 6; ++joint) {
      // Velocity's quadratic Bezier control hull bounds the whole interval;
      // linear acceleration takes its extrema at the endpoints.
      const double speed = std::max({std::abs(a.velocity[joint]), std::abs(middle[joint]),
                                     std::abs(b.velocity[joint])});
      scale = std::max(scale, speed / m_robot.timing.jointSpeed[joint]);
      scale = std::max(scale, std::sqrt(std::max(std::abs(acceleration0[joint]),
          std::abs(acceleration1[joint])) / m_robot.timing.jointAcceleration[joint]));
    }
    return scale;
  }

  std::optional<double> inspectInterval(qsizetype interval) const
  {
    const auto& first = m_data.points[interval];
    const auto& last = m_data.points[interval + 1];
    std::size_t phase = 1;
    while (phase < 5 && interval >= m_data.boundaries[phase + 1]) ++phase;
    const auto kind = static_cast<ChamferMotionPhase>(phase);
    const double from = interval == m_data.boundaries[phase] ? 0.0 : first.progress;
    const V3d chord = last.tcp.block<3, 1>(0, 3) - first.tcp.block<3, 1>(0, 3);
    const double length2 = chord.squaredNorm();
    if (!std::isfinite(length2) || length2 <= GeomConst::Eps * GeomConst::Eps) return std::nullopt;
    const double duration = last.time - first.time;
    const double step = std::min(1e-4, duration * 0.001);
    if (!std::isfinite(duration) || !(step > 1e-12)) return std::nullopt;
    double scale = jointScale(interval);
    for (int sample = 0; sample <= 16; ++sample) {
      const double u = static_cast<double>(sample) / 16.0;
      const V6d joints = interpolate(first, last, u);
      if (!withinLimits(joints, m_robot.model)) return std::nullopt;
      const M4d tcp = m_solver.solveFK(joints).back() * m_robot.flangeToTcp;
      if (!tcp.allFinite()) return std::nullopt;
      const V3d position = tcp.block<3, 1>(0, 3);
      const double fraction = std::clamp((position - first.tcp.block<3, 1>(0, 3)).dot(chord) / length2, 0.0, 1.0);
      const auto expected = centralTarget(m_path, kind, from + fraction * (last.progress - from));
      if (!expected || (position - expected->block<3, 1>(0, 3)).norm() > 0.01) return std::nullopt;
      const Eigen::Quaterniond actualRotation(M3d(tcp.block<3, 3>(0, 0)));
      const Eigen::Quaterniond expectedRotation(M3d(expected->block<3, 3>(0, 0)));
      if (actualRotation.angularDistance(expectedRotation) > 0.1 * GeomConst::DegToRad) return std::nullopt;
      // One segment's polynomial is extended only to estimate derivatives.
      // Checking both endpoints covers both sides of every acceleration jump.
      const V6d beforeJoints = interpolate(first, last, u - step / duration);
      const V6d afterJoints = interpolate(first, last, u + step / duration);
      const M4d before = m_solver.solveFK(beforeJoints).back() * m_robot.flangeToTcp;
      const M4d after = m_solver.solveFK(afterJoints).back() * m_robot.flangeToTcp;
      const V3d velocity = (after.block<3, 1>(0, 3) - before.block<3, 1>(0, 3)) / (2.0 * step);
      const V3d acceleration = ((after.block<3, 1>(0, 3) - position)
          + (before.block<3, 1>(0, 3) - position)) / (step * step);
      if (!velocity.allFinite() || !acceleration.allFinite()) return std::nullopt;
      scale = std::max(scale, velocity.norm() / feed(interval));
      scale = std::max(scale, std::sqrt(acceleration.norm() / m_robot.timing.cartesianAcceleration));
    }
    if (!std::isfinite(scale)) return std::nullopt;
    return scale;
  }

  bool timeCentral()
  {
    const auto first = m_data.boundaries[1];
    const auto last = m_data.boundaries[6];
    QVector<double> distance(last - first);
    QVector<double> speed(last - first + 1);
    const double acceleration = m_robot.timing.cartesianAcceleration;
    for (qsizetype i = first; i < last; ++i) {
      distance[i - first] = (m_data.points[i + 1].tcp.block<3, 1>(0, 3)
          - m_data.points[i].tcp.block<3, 1>(0, 3)).norm();
      if (!std::isfinite(distance[i - first]) || distance[i - first] <= GeomConst::Eps) return false;
      speed[i - first] = (i == first || i == m_data.boundaries[2] || i == m_data.boundaries[5])
          ? 0.0 : std::min(feed(i - 1), feed(i));
    }
    speed.back() = 0.0;
    for (qsizetype i = 1; i < speed.size(); ++i)
      speed[i] = std::min(speed[i], std::sqrt(speed[i - 1] * speed[i - 1] + 2.0 * acceleration * distance[i - 1]));
    for (qsizetype i = speed.size() - 2; i >= 0; --i)
      speed[i] = std::min(speed[i], std::sqrt(speed[i + 1] * speed[i + 1] + 2.0 * acceleration * distance[i]));
    for (qsizetype i = first; i < last; ++i) {
      const double sum = speed[i - first] + speed[i - first + 1];
      if (!(sum > 0.0)) return false;
      m_data.points[i + 1].time = m_data.points[i].time + 2.0 * distance[i - first] / sum;
    }
    setVelocities();
    double scale = 1.0;
    for (qsizetype i = first; i < last; ++i) {
      const auto required = inspectInterval(i);
      if (!required) {
        m_data.needsRefinement = true;
        m_data.error = QStringLiteral("Timed joint interpolation exceeds the geometric tolerance or is non-finite.");
        return false;
      }
      scale = std::max(scale, *required);
    }
    // Margin on numerical Cartesian estimates; joint bounds above are analytic.
    scale *= 1.05;
    const double start = m_data.points[first].time;
    for (qsizetype i = first; i <= last; ++i) {
      m_data.points[i].time = start + scale * (m_data.points[i].time - start);
      m_data.points[i].velocity /= scale;
    }
    m_data.centralTimeScale = scale;
    return true;
  }

  CompiledPoints m_data;
  const ChamferPath& m_path;
  const ChamferRobotSetup& m_robot;
  Kr10Kinematics m_solver;
};

} // namespace

ChamferMotion::ChamferMotion(const ChamferPath& path, const ChamferRobotSetup& robot,
                             QVector<ChamferJointPoint> points, const std::array<qsizetype, 8>& boundaries)
  : m_parameters(path.parameters()), m_robot(robot), m_points(std::move(points)), m_boundaries(boundaries) {}

ChamferMotionResult ChamferMotion::create(const ChamferPath& path, const ChamferRobotSetup& robot)
{
  if (!validRobot(robot)) return {{}, QStringLiteral("Invalid robot model, HOME limits or flange-to-TCP transform.")};
  if (!validTiming(robot.timing)) return {{}, QStringLiteral("Feeds and simulation limits must be finite and positive; auxiliary scale must be in (0, 1].")};
  MotionCompiler compiler(path, robot);
  const auto starts = compiler.starts();
  QString error = QStringLiteral("No IK solution for the approach P_s pose in the searched configurations.");
  for (const auto& start : starts) {
    for (int refinement = 0; refinement <= 3; ++refinement) {
      auto compiled = compiler.compile(start, refinement);
      if (!compiled.error.isEmpty()) { error = compiled.error; break; }
      auto timed = MotionTimer(std::move(compiled), path, robot).run();
      if (timed.error.isEmpty()) {
        ChamferMotion motion(path, robot, std::move(timed.points), timed.boundaries);
        motion.setCentralTimeScale(timed.centralTimeScale);
        return {std::move(motion), {}};
      }
      error = timed.error;
      if (!timed.needsRefinement) break;
    }
  }
  return {{}, error};
}

std::optional<ChamferPlaybackPose> ChamferMotion::evaluate(double seconds) const
{
  if (!std::isfinite(seconds) || m_points.isEmpty()) return std::nullopt;
  const double time = std::clamp(seconds, 0.0, duration());
  if (time == 0.0) return ChamferPlaybackPose{m_points.front().joints, m_points.front().tcp, ChamferMotionPhase::Approach};
  if (time == duration()) return ChamferPlaybackPose{m_points.back().joints, m_points.back().tcp, ChamferMotionPhase::ReturnHome};
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
    joints = interpolate(a, b, (time - a.time) / (b.time - a.time));
  }
  if (!withinLimits(joints, m_robot.model)) return std::nullopt;
  const M4d tcp = Kr10Kinematics(m_robot.model).solveFK(joints).back() * m_robot.flangeToTcp;
  if (!tcp.allFinite()) return std::nullopt;
  return ChamferPlaybackPose{joints, tcp, static_cast<ChamferMotionPhase>(phase)};
}
