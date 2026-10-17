#include "chamfercentralvalidation.h"

#include "geometry/utils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {

using namespace RoboCrap3D;
constexpr qsizetype MaximumSamples = 200000;
constexpr double PreviewPositionTolerance = 0.01; // mm; Cartesian chord criterion only.
constexpr double RateStep = 1e-4;
constexpr double RateTolerance = 1e-6;

bool positiveFinite(double value)
{
  return std::isfinite(value) && value > 0.0;
}

struct ColumnStretch
{
  double minimum = 0.0;
  double maximum = 0.0;
};

// Singular values of a 3x2 matrix from its columns, without iterative decomposition.
std::optional<ColumnStretch> columnStretch(const V3d& first, const V3d& second)
{
  const double scale = std::max(first.stableNorm(), second.stableNorm());
  if (!first.allFinite() || !second.allFinite() || !positiveFinite(scale)) return std::nullopt;
  const V3d left = first / scale;
  const V3d right = second / scale;
  const double aa = left.squaredNorm();
  const double bb = right.squaredNorm();
  const double ab = left.dot(right);
  const double maximum = std::sqrt(0.5 * (aa + bb + std::hypot(aa - bb, 2.0 * ab)));
  // Product of singular values equals the parallelogram area; avoids cancellation.
  const double minimum = left.cross(right).stableNorm() / maximum;
  if (!positiveFinite(minimum) || minimum <= GeomConst::Eps * maximum) return std::nullopt;
  const ColumnStretch result{scale * minimum, scale * maximum};
  if (!positiveFinite(result.minimum) || !positiveFinite(result.maximum)) return std::nullopt;
  return result;
}

struct MachiningDerivativeBounds
{
  double first = 0.0; // Upper norm bound, mm per radian.
  double second = 0.0; // Upper norm bound, mm per radian squared.
  double axialFirst = 0.0; // Upper axial first-derivative magnitude, mm per radian.
  double transverseFirst = 0.0; // Lower transverse first-derivative norm, mm per radian.
};

// Coefficient-based bounds derived in codex_plans/SPATIAL_LEADS_CURVATURE_STEP1_RU.md.
// Includes the chamfer offset and the actual (rounded) ellipse projection coefficients.
std::optional<MachiningDerivativeBounds> machiningDerivativeBounds(const ChamferPath& path)
{
  const auto& parameters = path.parameters();
  const auto& edge = parameters.edge;
  const V3d& axis = path.outwardAxis();
  const V3d major = edge.majorRadius * edge.majorAxis;
  const V3d minor = edge.minorRadius * edge.minorAxis;
  const auto full = columnStretch(major, minor);
  const auto projected = columnStretch(major - major.dot(axis) * axis,
                                        minor - minor.dot(axis) * axis);
  if (!full || !projected) return std::nullopt;
  const V3d normal = edge.majorAxis.cross(edge.minorAxis).normalized();
  const double slope = normal.cross(axis).stableNorm() / std::abs(normal.dot(axis));
  const double ratio = std::max(1.0, projected->maximum / projected->minimum);
  const double ratioSquared = ratio * ratio;
  const double radialAngularSecond = ratio * (ratioSquared - 1.0);
  const double radialSecond = ratioSquared + radialAngularSecond;
  const double firstG = slope * ratio;
  const double secondG = 2.0 * firstG * firstG + slope * radialSecond;
  const double cotangent = 1.0 / std::tan(parameters.chamferAngleDegrees * GeomConst::DegToRad);
  const double firstAxialFactor = firstG * (2.0 * slope + cotangent) + 2.0 * slope * ratio;
  const double secondAxialFactor = secondG * (2.0 * slope + cotangent)
      + 4.0 * firstG * slope * ratio + 2.0 * slope * radialSecond;
  const double halfSize = 0.5 * parameters.chamferSize;
  const MachiningDerivativeBounds result{
      full->maximum + halfSize * (firstG + ratio + firstAxialFactor),
      full->maximum + halfSize * (secondG + 2.0 * firstG * ratio + radialSecond + secondAxialFactor),
      std::hypot(major.dot(axis), minor.dot(axis)) + halfSize * firstAxialFactor,
      (projected->minimum + halfSize / std::hypot(1.0, slope)) / ratio};
  if (!positiveFinite(result.first) || !positiveFinite(result.second)
      || !std::isfinite(result.axialFirst) || result.axialFirst < 0.0
      || !positiveFinite(result.transverseFirst)) return std::nullopt;
  return result;
}

std::optional<double> leadCurvatureBound(const MachiningDerivativeBounds& bounds,
                                       const ChamferLeadParameters& lead)
{
  const double span = lead.spanDegrees * GeomConst::DegToRad;
  const double transverse = bounds.transverseFirst * span;
  const double axial = bounds.axialFirst * span;
  const double clearance = lead.clearance;
  if (!positiveFinite(transverse) || !std::isfinite(axial) || axial < 0.0
      || !positiveFinite(clearance)) return std::nullopt;
  // Scale the Gram eigenvalue formula to avoid squared/fourth-power overflow.
  const double scale = std::max({transverse, axial, clearance});
  const double x = transverse / scale;
  const double y = axial / scale;
  const double z = clearance / scale;
  const double sum = x * x + y * y + z * z;
  const double discriminant = std::hypot(x - z, y) * std::hypot(x + z, y);
  const double minimum = (x * z) * std::sqrt(2.0 / (sum + discriminant));
  if (!positiveFinite(minimum) || minimum <= GeomConst::Eps * std::sqrt(sum))
    return std::nullopt;
  const double firstLower = scale * minimum;
  const double secondUpper = bounds.second * span * span + bounds.first * span + clearance;
  const double curvature = (secondUpper / firstLower) / firstLower;
  if (!positiveFinite(firstLower) || !positiveFinite(secondUpper) || !positiveFinite(curvature))
    return std::nullopt;
  return curvature;
}

std::optional<std::array<double, 3>> phaseCurvatureBounds(const ChamferPath& path)
{
  const auto bounds = machiningDerivativeBounds(path);
  if (!bounds) return std::nullopt;
  const auto leadIn = leadCurvatureBound(*bounds, path.parameters().leadIn);
  const auto leadOut = leadCurvatureBound(*bounds, path.parameters().leadOut);
  const double machining = (bounds->second / bounds->transverseFirst) / bounds->transverseFirst;
  if (!leadIn || !leadOut || !positiveFinite(machining)) return std::nullopt;
  return std::array<double, 3>{*leadIn, machining, *leadOut};
}

struct PreviewSchedule
{
  QVector<ChamferCentralPreviewSpan> spans;
  qsizetype points = 1; // Shared starts/ends are stored only once.
  QString error;
};

PreviewSchedule makePreviewSchedule(const ChamferPath& path, const ChamferCentralMotion& motion)
{
  const auto curvature = phaseCurvatureBounds(path);
  if (!curvature) return {{}, 0, QStringLiteral("Cannot bound TCP curvature: degenerate projection, ill-conditioned lead dimensions or numerical range exceeded.")};
  PreviewSchedule schedule;
  for (int index = 0; index < 3; ++index) {
    const auto phase = static_cast<ChamferPhase>(index);
    const auto profile = motion.profile(phase);
    if (!profile) return {{}, 0, QStringLiteral("Missing preview phase profile.")};
    const double start = motion.phaseTimes()[index];
    const double end = motion.phaseTimes()[index + 1];
    QVector<double> stops{start};
    if (index != 1 && profile->cruiseSeconds > 0.0)
      stops.append(start + (index == 0 ? profile->rampSeconds : profile->cruiseSeconds));
    stops.append(end);
    const double normalAcceleration = profile->speed * (profile->speed
        * (*curvature)[index]);
    for (qsizetype part = 1; part < stops.size(); ++part) {
      const bool ramp = index == 0 ? part == 1 : index == 2 && part == stops.size() - 1;
      const double bound = std::hypot(ramp ? profile->acceleration : 0.0, normalAcceleration);
      const double duration = stops[part] - stops[part - 1];
      const double count = std::max(1.0, std::ceil(duration
          * std::sqrt(bound / (8.0 * PreviewPositionTolerance))));
      if (!std::isfinite(bound) || bound <= 0.0 || !std::isfinite(duration) || duration <= 0.0
          || !std::isfinite(count) || count > MaximumSamples - schedule.points)
        return {{}, 0, QStringLiteral("Preview geometric schedule exceeds numerical range or 200000 points.")};
      const int intervals = static_cast<int>(count);
      if (!(duration / intervals > 1e-9))
        return {{}, 0, QStringLiteral("Preview geometric interval is below time resolution.")};
      schedule.spans.append({stops[part - 1], stops[part], intervals,
          index == 0 ? QStringLiteral("leadIn") : index == 1 ? QStringLiteral("machining")
                                                           : QStringLiteral("leadOut")});
      schedule.points += intervals;
    }
  }
  return schedule;
}

bool rigid(const M4d& pose)
{
  return pose.allFinite()
      && isBasis(pose.block<3, 1>(0, 0), pose.block<3, 1>(0, 1),
                 pose.block<3, 1>(0, 2), Kr10RotEps)
      && (pose.row(3) - Eigen::RowVector4d(0, 0, 0, 1)).cwiseAbs().maxCoeff() <= GeomConst::Eps;
}

bool validRobot(const ChamferRobotSetup& robot)
{
  const auto& model = robot.model;
  const auto& ik = model.ik;
  const auto& timing = robot.timing;
  if (!rigid(robot.flangeToTcp) || !model.qHome.allFinite()
      || !std::isfinite(ik.sx) || !std::isfinite(ik.sz) || !std::isfinite(ik.a)
      || !std::isfinite(ik.bx) || !std::isfinite(ik.by) || !std::isfinite(ik.dF)
      || !Kr10Kinematics(model).isValid() || !timing.jointSpeed.allFinite()
      || timing.jointSpeed.minCoeff() <= 0.0 || !timing.jointAcceleration.allFinite()
      || timing.jointAcceleration.minCoeff() <= 0.0
      || !std::isfinite(timing.cartesianAcceleration) || timing.cartesianAcceleration <= 0.0)
    return false;
  for (const auto& joint : model.joints)
    if (!rigid(joint.localTransform) || !joint.axis.allFinite()
        || std::abs(joint.axis.x()) > GeomConst::Eps || std::abs(joint.axis.y()) > GeomConst::Eps
        || std::abs(std::abs(joint.axis.z()) - 1.0) > GeomConst::Eps
        || !std::isfinite(joint.qMin) || !std::isfinite(joint.qMax) || joint.qMin > joint.qMax
        || std::abs(joint.qMin) > 1e6 || std::abs(joint.qMax) > 1e6
        || !std::isfinite(joint.ratedPayloadSpeedDegreesPerSecond)
        || joint.ratedPayloadSpeedDegreesPerSecond <= 0.0)
      return false;
  return true;
}

QString positionError(const V6d& joints, const Kr10KinematicModel& model)
{
  if (!joints.allFinite()) return QStringLiteral("Nonfinite joint coordinates.");
  for (std::size_t axis = 0; axis < DofCount; ++axis) {
    const auto& joint = model.joints[axis];
    if (joints[axis] < joint.qMin || joints[axis] > joint.qMax)
      return QStringLiteral("A%1 position %2 deg is outside [%3, %4].")
          .arg(axis + 1).arg(joints[axis], 0, 'g', 12).arg(joint.qMin).arg(joint.qMax);
  }
  return {};
}

struct SampleResult
{
  std::optional<ChamferCentralJointSample> sample;
  QString error;
};

struct Rates
{
  V6d joints = V6d::Zero();
  V3d xyz = V3d::Zero();
  double seconds = 0.0;
};

class CentralValidator
{
public:
  // Borrow the Generate worker's immutable inputs; this solver never outlives them.
  CentralValidator(const ChamferCentralPreparation& preparation, const ChamferRobotSetup& robot)
    : m_preparation(preparation), m_robot(robot), m_solver(robot.model),
      m_tcpToFlange(inverseRigidTransform(m_robot.flangeToTcp)) {}

  ChamferCentralValidationResult run(const Kr10Kinematics::IkSolution& seed)
  {
    m_samples.reserve(m_preparation.plannedSamples());
    const auto first = sample(0.0, seed);
    if (!first.sample) return {{}, atTime(first.error, 0.0)};
    m_samples.append(*first.sample);
    for (const auto& span : m_preparation.spans()) {
      QString error;
      double time = span.start;
      for (int index = 1; index <= span.intervals; ++index) {
        time = index == span.intervals ? span.end
            : span.start + (span.end - span.start) * (double(index) / span.intervals);
        error = appendPoint(time);
        if (!error.isEmpty()) break;
      }
      if (!error.isEmpty())
        return {{}, span.phase + QStringLiteral(": ") + atTime(error, time)};
    }
    const QString error = inspectRates();
    if (!error.isEmpty()) return {{}, error};
    return {ChamferCentralValidationData{std::move(m_samples)}, {}};
  }

private:
  QString atTime(const QString& error, double time) const
  {
    return QStringLiteral("%1 (central t=%2 s)").arg(error).arg(time, 0, 'g', 12);
  }

  SampleResult sample(double time, const Kr10Kinematics::IkSolution& seed)
  {
    const auto target = m_preparation.evaluateTarget(m_samples.size(), time);
    if (!target) return {{}, QStringLiteral("Cannot evaluate prescribed TCP target.")};
    const M4d flange = target->tcp * m_tcpToFlange;
    const auto solution = m_solver.calculateIK(flange, seed);
    if (!solution) return {{}, QStringLiteral("IK failed: unreachable, singular or outside the seeded configuration/limits.")};
    const QString error = positionError(solution->q, m_robot.model);
    if (!error.isEmpty()) return {{}, error};
    const auto transforms = m_solver.solveFK(solution->q);
    if (!Kr10Kinematics::matchesFlange(flange, transforms.back()))
      return {{}, QStringLiteral("Flange FK reconstruction exceeds 0.0001 mm or 0.000001 rotation-entry tolerance.")};
    const M4d actual = transforms.back() * m_robot.flangeToTcp;
    if (!actual.allFinite()
        || (actual.block<3, 1>(0, 3) - target->tcp.block<3, 1>(0, 3)).norm() > Kr10PosEps
        || (actual.block<3, 3>(0, 0) - target->tcp.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
      return {{}, QStringLiteral("TCP FK reconstruction exceeds 0.0001 mm or 0.000001 rotation-entry tolerance.")};
    return {ChamferCentralJointSample{time, *target,
        m_solver.configuration(solution->q, transforms), actual}, {}};
  }

  QString appendPoint(double time)
  {
    const auto& previous = m_samples.back();
    const double step = time - previous.time;
    if (!(step > 1e-9)) return QStringLiteral("Preview timestamps lost numerical resolution.");
    const auto current = sample(time, previous.solution);
    if (!current.sample) return current.error;
    const QString error = inspectSpeed((current.sample->solution.q - previous.solution.q) / step);
    if (!error.isEmpty()) return error;
    m_samples.append(*current.sample);
    return {};
  }

  QString accelerationError(const Rates& before, const Rates& after) const
  {
    const double elapsed = 0.5 * (before.seconds + after.seconds);
    if (!(elapsed > 0.0)) return QStringLiteral("Unresolved acceleration interval.");
    const V6d acceleration = (after.joints - before.joints) / elapsed;
    const V3d xyz = (after.xyz - before.xyz) / elapsed;
    if (!acceleration.allFinite() || !xyz.allFinite()) return QStringLiteral("Nonfinite sampled acceleration.");
    // Temporary Dry Run diagnostic: per-joint acceleration rejection disabled.
    // for (int axis = 0; axis < 6; ++axis)
    //   if (std::abs(acceleration[axis]) > m_robot.timing.jointAcceleration[axis] * (1.0 + RateTolerance))
    //     return QStringLiteral("A%1 acceleration %2 deg/s^2 exceeds configured simulation bound %3.")
    //         .arg(axis + 1).arg(std::abs(acceleration[axis]), 0, 'g', 12)
    //         .arg(m_robot.timing.jointAcceleration[axis]);
    if (xyz.norm() > m_robot.timing.cartesianAcceleration * (1.0 + RateTolerance))
      return QStringLiteral("XYZ acceleration %1 mm/s^2 exceeds configured simulation bound %2.")
          .arg(xyz.norm(), 0, 'g', 12).arg(m_robot.timing.cartesianAcceleration);
    return {};
  }

  QString inspectSpeed(const V6d& velocity) const
  {
    if (!velocity.allFinite()) return QStringLiteral("Nonfinite sampled joint velocity.");
    for (int axis = 0; axis < 6; ++axis) {
      const double limit = std::min(m_robot.timing.jointSpeed[axis],
          m_robot.model.joints[axis].ratedPayloadSpeedDegreesPerSecond);
      if (std::abs(velocity[axis]) > limit * (1.0 + RateTolerance))
        return QStringLiteral("A%1 speed %2 deg/s exceeds effective bound %3.")
            .arg(axis + 1).arg(std::abs(velocity[axis]), 0, 'g', 12).arg(limit);
    }
    return {};
  }

  QString inspectRates() const
  {
    auto first = m_samples.front();
    Rates previous; // Rest at t=0; interval velocities are located at their midpoints.
    for (qsizetype index = 1; index < m_samples.size(); ++index) {
      const auto& last = m_samples[index];
      const bool terminal = index == m_samples.size() - 1;
      const double step = last.time - first.time;
      if (!terminal && (step < RateStep || m_preparation.motion().duration() - last.time < RateStep)) continue;
      if (!(step >= RateStep)) return QStringLiteral("Central motion is too short for resolved rate validation.");
      Rates current;
      current.seconds = step;
      current.joints = (last.solution.q - first.solution.q) / step;
      current.xyz = (last.target.tcp.block<3, 1>(0, 3) - first.target.tcp.block<3, 1>(0, 3)) / step;
      if (!current.joints.allFinite() || !current.xyz.allFinite())
        return atTime(QStringLiteral("Nonfinite sampled velocity."), last.time);
      const QString speedError = inspectSpeed(current.joints);
      if (!speedError.isEmpty()) return atTime(speedError, last.time);
      const QString error = accelerationError(previous, current);
      if (!error.isEmpty()) return atTime(error, last.time);
      previous = current;
      first = last;
    }
    const QString stop = accelerationError(previous, Rates{});
    return stop.isEmpty() ? QString{} : atTime(stop, m_preparation.motion().duration());
  }

  const ChamferCentralPreparation& m_preparation;
  const ChamferRobotSetup& m_robot;
  Kr10Kinematics m_solver;
  M4d m_tcpToFlange = M4d::Identity();
  QVector<ChamferCentralJointSample> m_samples;
};

} // namespace

ChamferCentralPreparation::ChamferCentralPreparation(const ChamferPath& path,
                                                     const ChamferTimingParameters& timing)
  : m_path(path), m_timing(timing) {}

QString ChamferCentralPreparation::prepare()
{
  if (m_attempted) return m_error;
  m_attempted = true;
  auto motion = ChamferCentralMotion::create(m_path, m_timing);
  m_error = motion.error;
  if (motion.motion) {
    m_motion = std::move(motion.motion);
    auto schedule = makePreviewSchedule(m_path, *m_motion);
    m_error = schedule.error;
    if (m_error.isEmpty()) {
      m_spans = std::move(schedule.spans);
      m_plannedSamples = schedule.points;
    }
  }
  return m_error;
}

ChamferCentralValidationResult ChamferCentralPreparation::validate(const ChamferRobotSetup& robot,
                                                                  const Kr10Kinematics::IkSolution& initialSeed)
{
  if (!validRobot(robot)) return {{}, QStringLiteral("Invalid robot model, TCP calibration or simulation rate bounds.")};
  const QString error = positionError(initialSeed.q, robot.model);
  if (!error.isEmpty()) return {{}, QStringLiteral("Invalid initial IK seed: %1").arg(error)};
  const QString preparationError = prepare();
  if (!preparationError.isEmpty()) return {{}, preparationError};
  return CentralValidator(*this, robot).run(initialSeed);
}

std::optional<ChamferCentralPose> ChamferCentralPreparation::evaluateTarget(qsizetype index,
                                                                         double seconds) const
{
  if (index >= 0 && index < m_targets.size()) return m_targets[index];
  if (!m_motion || index != m_targets.size() || index >= m_plannedSamples)
    return std::nullopt;
  const auto target = m_motion->evaluate(seconds);
  m_targets.append(target); // A failed target remains at this exact scheduled index.
  return target;
}
