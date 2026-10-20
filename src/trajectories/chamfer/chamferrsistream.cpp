#include "trajectories/chamferrsistream.h"

#include "chamfertcpsolver.h"
#include "geometry/utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSaveFile>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

using namespace RoboCrap3D;

constexpr double CyclePeriod = 0.004; // s, RSI IPO_FAST controller cycle.
constexpr int SettleCycles = 50; // Zero corrections before Result = 1.
constexpr double MaximumCycles = 200000.0;
constexpr double WireScale = 1e6; // Six fixed decimal places on the wire.
constexpr double RateTolerance = 1e-6;

V3d principalEulerDegrees(const M3d& rotation)
{
  // atan2 retains pitch accuracy near +/-90 degrees, where asin can lose it.
  const double cosinePitch = std::hypot(rotation(0, 0), rotation(1, 0));
  if (cosinePitch <= GeomConst::Eps)
    return {std::atan2(-rotation(0, 1), rotation(1, 1)) * GeomConst::RadToDeg,
            std::copysign(90.0, -rotation(2, 0)), 0.0};
  return {std::atan2(rotation(1, 0), rotation(0, 0)) * GeomConst::RadToDeg,
          std::atan2(-rotation(2, 0), cosinePitch) * GeomConst::RadToDeg,
          std::atan2(rotation(2, 1), rotation(2, 2)) * GeomConst::RadToDeg};
}

double rotationEntryError(const M3d& target, const V3d& angles)
{
  return (euler2rot(angles[0], angles[1], angles[2]) - target).cwiseAbs().maxCoeff();
}

// The value the controller parses from six fixed decimal places.
V6d wireValues(const V6d& values)
{
  return ((values * WireScale).array().round() / WireScale).matrix();
}

QString atTime(const QString& error, double seconds)
{
  return QStringLiteral("%1 (central t=%2 s)").arg(error).arg(seconds, 0, 'f', 6);
}

QString csvValues(const V6d& values)
{
  QStringList fields;
  for (double value : values) fields.append(QString::number(value, 'f', 9));
  return fields.join(QLatin1Char(','));
}

class RsiStreamSampler
{
public:
  // The saved motion outlives this worker-local sampler.
  explicit RsiStreamSampler(const ChamferMotion& motion)
    : m_motion(motion), m_solver(motion.robot()) {}

  ChamferRsiStreamResult run()
  {
    const auto& central = m_motion.centralMotion();
    if (!central) return {{}, QStringLiteral("Regenerate the path before RSI Sampling: analytical timing is missing.")};
    const auto& points = m_motion.points();
    const auto& boundaries = m_motion.boundaries();
    // Generate bounds each saved central interval h by a*h^2/8 <= 0.01 mm.
    // A controller cycle no longer than every h keeps its chord within the same bound.
    double shortestInterval = std::numeric_limits<double>::infinity();
    for (qsizetype index = boundaries[2] + 1; index <= boundaries[5]; ++index)
      shortestInterval = std::min(shortestInterval, points[index].time - points[index - 1].time);
    if (!(shortestInterval >= CyclePeriod))
      return {{}, QStringLiteral("Machining speed is too high for the 4 ms RSI chord tolerance (shortest Generate interval %1 ms).")
                      .arg(shortestInterval * 1e3, 0, 'g', 6)};
    const double duration = central->duration();
    double cycles = std::ceil(duration / CyclePeriod);
    if (cycles * CyclePeriod < duration) ++cycles;
    if (cycles > 1.0 && (cycles - 1.0) * CyclePeriod >= duration) --cycles;
    if (!std::isfinite(cycles) || cycles < 1.0 || cycles > MaximumCycles)
      return {{}, QStringLiteral("RSI sampling exceeds the 200000-cycle limit.")};
    const auto count = static_cast<qsizetype>(cycles);

    const V6d& startJoints = points[boundaries[2]].joints;
    const auto& kinematics = m_solver.kinematics();
    m_seed = kinematics.configuration(startJoints, kinematics.solveFK(startJoints));
    m_stream.duration = duration;
    m_stream.increments.reserve(count + SettleCycles);
    m_stream.diagnosticId = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    const QDir records(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("records")));
    m_trace.setFileName(records.filePath(QStringLiteral("rsi_plan_%1.csv")
        .arg(m_stream.diagnosticId)));
    if (!QDir().mkpath(records.absolutePath()) || !m_trace.open(QIODevice::WriteOnly)) {
      qInfo().noquote() << "[ERROR] RSI Sampling: cannot open diagnostic" << m_trace.fileName()
                        << m_trace.errorString();
    }
    writeTrace(QStringLiteral("# plan_id,") + m_stream.diagnosticId
        + QStringLiteral("\n# XYZ mm; ABC/joints deg; joints from rounded offsets; time s; cycle 0.004 s\n# staging_joints_deg,")
        + csvValues(points[boundaries[1]].joints)
        + QStringLiteral("\n# joint_speed_limits_deg_s,") + csvValues(m_motion.robot().timing.jointSpeed)
        + QStringLiteral("\ntime_s,target_X,target_Y,target_Z,target_A,target_B,target_C,")
        + QStringLiteral("joint_A1,joint_A2,joint_A3,joint_A4,joint_A5,joint_A6,")
        + QStringLiteral("increment_X,increment_Y,increment_Z,increment_A,increment_B,increment_C\n"));
    for (qsizetype index = 0; index <= count; ++index) {
      const double time = index == count ? duration : index * CyclePeriod;
      const QString error = index == 0 ? startCycle() : appendCycle(time);
      if (!error.isEmpty()) return {{}, atTime(error, time)};
    }
    // The cycle after the last correction carries zero motion.
    const QString stopError = accelerationError(V3d::Zero());
    if (!stopError.isEmpty()) return {{}, atTime(stopError, duration)};
    const V6d& endJoints = points[boundaries[5]].joints;
    const auto expected = kinematics.configuration(endJoints, kinematics.solveFK(endJoints));
    // TCP accuracy was checked above; compare branch and the nearest 360-degree representatives.
    if (m_seed.status != expected.status || (m_seed.q - endJoints).cwiseAbs().maxCoeff() >= 180.0)
      return {{}, QStringLiteral("RSI endpoint does not retain the saved joint configuration/winding.")};

    // The controller must use the TCP the corrections were computed for.
    const M4d& flangeToTcp = m_motion.robot().flangeToTcp;
    const M3d toolRotation = flangeToTcp.block<3, 3>(0, 0);
    const V3d toolAngles = principalEulerDegrees(toolRotation);
    // ChamferMotion::create already accepted flangeToTcp as a finite rigid transform.
    if (!toolAngles.allFinite() || rotationEntryError(toolRotation, toolAngles) > Kr10RotEps)
      return {{}, QStringLiteral("Cannot represent the saved flange-to-TCP orientation as A/B/C.")};
    m_stream.toolCoordinates << flangeToTcp.block<3, 1>(0, 3), toolAngles;

    m_stream.motionCycles = static_cast<int>(m_stream.increments.size());
    m_stream.increments.insert(m_stream.increments.size(), SettleCycles, V6d::Zero());
    const M3d finalRotation = m_reconstructedTcp.block<3, 3>(0, 0);
    m_stream.finalCoordinates << m_reconstructedTcp.block<3, 1>(0, 3),
                                 principalEulerDegrees(finalRotation);
    m_stream.finalCoordinates = wireValues(m_stream.finalCoordinates);
    if (!m_stream.finalCoordinates.allFinite()
        || rotationEntryError(finalRotation, m_stream.finalCoordinates.tail<3>()) > Kr10RotEps)
      return {{}, QStringLiteral("Cannot represent the reconstructed final TCP orientation as A/B/C.")};
    m_stream.stagingPoint = central->path().stagingPoint();
    writeTrace(QStringLiteral("# tool_xyzabc,") + csvValues(m_stream.toolCoordinates)
        + QStringLiteral("\n# phase_end_times_s,") + QString::number(central->phaseTimes()[1], 'f', 9)
        + QLatin1Char(',') + QString::number(central->phaseTimes()[2], 'f', 9)
        + QLatin1Char(',') + QString::number(duration, 'f', 9)
        + QStringLiteral("\n# settle_zero_cycles,") + QString::number(SettleCycles) + QLatin1Char('\n'));
    if (m_trace.isOpen()) {
      if (m_trace.commit())
        qInfo().noquote() << "[OK] RSI Sampling: saved diagnostic" << m_trace.fileName();
      else
        qInfo().noquote() << "[ERROR] RSI Sampling: cannot save diagnostic" << m_trace.fileName()
                          << m_trace.errorString();
    }
    return {std::move(m_stream), {}};
  }

private:
  QString startCycle()
  {
    const auto target = m_motion.centralMotion()->evaluate(0.0);
    if (!target) return QStringLiteral("Cannot evaluate the prescribed TCP target.");
    const M3d rotation = target->tcp.block<3, 3>(0, 0);
    m_stream.initialCoordinates << target->tcp.block<3, 1>(0, 3), principalEulerDegrees(rotation);
    m_stream.initialCoordinates = wireValues(m_stream.initialCoordinates);
    if (!m_stream.initialCoordinates.allFinite()) return QStringLiteral("Nonfinite initial RSI pose.");
    const auto angles = m_stream.initialCoordinates.tail<3>();
    m_initialRotation = euler2rot(angles[0], angles[1], angles[2]);
    if ((m_stream.initialCoordinates.head<3>() - target->tcp.block<3, 1>(0, 3)).norm() > Kr10PosEps
        || (m_initialRotation - rotation).cwiseAbs().maxCoeff() > Kr10RotEps)
      return QStringLiteral("Cannot represent the initial TCP pose at six decimal places.");
    m_reconstructedTcp = makeTransform(m_initialRotation, m_stream.initialCoordinates.head<3>());
    const auto inverse = m_solver.solveTcp(m_reconstructedTcp, m_seed);
    if (!inverse.solution) return inverse.error;
    m_seed = *inverse.solution;
    recordSample(0.0, target->tcp);
    return {};
  }

  QString appendCycle(double time)
  {
    const auto target = m_motion.centralMotion()->evaluate(time);
    if (!target) return QStringLiteral("Cannot evaluate the prescribed TCP target.");
    const V3d position = target->tcp.block<3, 1>(0, 3);
    const M3d relativeRotation = target->tcp.block<3, 3>(0, 0)
        * m_reconstructedTcp.block<3, 3>(0, 0).transpose();
    V6d increment = V6d::Zero();
    increment << position - m_reconstructedTcp.block<3, 1>(0, 3),
                 principalEulerDegrees(relativeRotation);
    increment = wireValues(increment);
    if (!increment.allFinite()) return QStringLiteral("Nonfinite RSI correction.");
    // WORLD offsets rotate at the TCP; XYZ translation is added independently.
    const M3d rotation = euler2rot(increment[3], increment[4], increment[5])
        * m_reconstructedTcp.block<3, 3>(0, 0);
    const V3d reconstructedPosition = m_reconstructedTcp.block<3, 1>(0, 3) + increment.head<3>();
    const M4d reconstructed = makeTransform(rotation, reconstructedPosition);
    if (!reconstructed.allFinite() || (reconstructedPosition - position).norm() > Kr10PosEps
        || (rotation - target->tcp.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
      return QStringLiteral("Rounded RSI offsets exceed the TCP reconstruction tolerance.");
    const auto inverse = m_solver.solveTcp(reconstructed, m_seed);
    if (!inverse.solution) return inverse.error;
    const QString speedError = chamferJointSpeedError(
        (inverse.solution->q - m_seed.q) / CyclePeriod, m_motion.robot().timing.jointSpeed);
    if (!speedError.isEmpty()) return speedError;
    const V3d velocity = (reconstructedPosition - m_reconstructedTcp.block<3, 1>(0, 3)) / CyclePeriod;
    const QString accelerationFailure = accelerationError(velocity);
    if (!accelerationFailure.isEmpty()) return accelerationFailure;
    m_reconstructedTcp = reconstructed;
    m_stream.increments.append(increment);
    m_stream.maximumTranslationOffset = std::max(m_stream.maximumTranslationOffset,
        (reconstructedPosition - m_stream.initialCoordinates.head<3>()).cwiseAbs().maxCoeff());
    m_stream.maximumRotationOffset = std::max(m_stream.maximumRotationOffset,
        Eigen::AngleAxisd(rotation * m_initialRotation.transpose()).angle() * GeomConst::RadToDeg);
    m_seed = *inverse.solution;
    m_previousVelocity = velocity;
    recordSample(time, target->tcp);
    return {};
  }

  void recordSample(double time, const M4d& target)
  {
    if (!m_trace.isOpen()) return;
    V6d coordinates = V6d::Zero();
    coordinates << target.block<3, 1>(0, 3), principalEulerDegrees(target.block<3, 3>(0, 0));
    V6d increment = V6d::Zero();
    if (!m_stream.increments.isEmpty()) increment = m_stream.increments.back();
    writeTrace(QString::number(time, 'f', 9) + QLatin1Char(',') + csvValues(coordinates)
        + QLatin1Char(',') + csvValues(m_seed.q) + QLatin1Char(',') + csvValues(increment)
        + QLatin1Char('\n'));
  }

  void writeTrace(const QString& text)
  {
    if (!m_trace.isOpen()) return;
    const QByteArray bytes = text.toUtf8();
    if (m_trace.write(bytes) == bytes.size()) return;
    qInfo().noquote() << "[ERROR] RSI Sampling: cannot write diagnostic" << m_trace.fileName()
                      << m_trace.errorString();
    m_trace.cancelWriting();
    m_trace.commit(); // Close and discard the cancelled temporary file.
  }

  // Change from the previous cycle's TCP velocity; the first cycle starts at rest.
  QString accelerationError(const V3d& velocity) const
  {
    const double acceleration = (velocity - m_previousVelocity).norm() / CyclePeriod;
    const double limit = m_motion.robot().timing.cartesianAcceleration;
    if (!std::isfinite(acceleration) || acceleration > limit * (1.0 + RateTolerance))
      return QStringLiteral("TCP acceleration %1 mm/s^2 exceeds bound %2.")
          .arg(acceleration, 0, 'g', 12).arg(limit);
    return {};
  }

  const ChamferMotion& m_motion;
  ChamferTcpSolver m_solver;
  ChamferRsiStream m_stream;
  QSaveFile m_trace; // Worker-local planned values; never written by ControlIO.
  Kr10Kinematics::IkSolution m_seed;
  M4d m_reconstructedTcp = M4d::Identity(); // Pose after the rounded offsets already appended.
  M3d m_initialRotation = M3d::Identity(); // Fixed reference for the summary's angular excursion.
  V3d m_previousVelocity = V3d::Zero();
};

} // namespace

ChamferRsiStreamResult sampleChamferRsiStream(const ChamferMotion& motion)
{
  return RsiStreamSampler(motion).run();
}
