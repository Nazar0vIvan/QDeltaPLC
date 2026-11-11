#include "trajectories/probersistream.h"

#include "../motion/rsicoordinates.h"
#include "geometry/utils.h"
#include "kinematics/kr10kinematics.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSaveFile>
#include <QStringList>

#include <cmath>
#include <utility>

namespace {

using namespace RoboCrap3D;

constexpr qsizetype StepLimit = 600; // Array size in RSI_Probe_Calibration.dat.
constexpr double CyclePeriod = 0.004; // s, RSI IPO_FAST controller cycle.
constexpr double SearchDistance = 5.0; // mm from the search start: 3 mm to the nominal contact plus 2 mm.
constexpr int SettleCycles = 50; // Zero corrections before Result = 1, as in the other streams.

// Six-decimal XYZ and principal ABC of a pose; empty when the rounded values miss the pose by more
// than the KR10 position or rotation-entry tolerance.
std::optional<V6d> wirePose(const M4d& pose)
{
  V6d coordinates = V6d::Zero();
  coordinates << pose.block<3, 1>(0, 3), principalEulerDegrees(pose.block<3, 3>(0, 0));
  coordinates = wireValues(coordinates);
  const M3d rotation = euler2rot(coordinates[3], coordinates[4], coordinates[5]);
  if (!coordinates.allFinite() || (coordinates.head<3>() - pose.block<3, 1>(0, 3)).norm() > Kr10PosEps
      || (rotation - pose.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() > Kr10RotEps)
    return std::nullopt;
  return coordinates;
}

// Constant feed along the approach direction -normal for SearchDistance, then settle zeros. Each
// increment is the difference of consecutive rounded offsets, so the increments add up exactly to
// the rounded end offset.
QVector<V6d> searchStream(const V3d& normal, double feed)
{
  const double step = feed * CyclePeriod;
  const int cycles = static_cast<int>(std::ceil(SearchDistance / step - 1e-9));
  QVector<V6d> stream(cycles + SettleCycles, V6d::Zero());
  V6d previous = V6d::Zero();
  for (int cycle = 0; cycle < cycles; ++cycle) {
    V6d offset = V6d::Zero();
    offset.head<3>() = -(cycle + 1) * step * normal;
    offset = wireValues(offset);
    stream[cycle] = offset - previous;
    previous = offset;
  }
  return stream;
}

QString csvPose(const M4d& pose)
{
  QStringList fields;
  for (int axis = 0; axis < 3; ++axis) fields.append(QString::number(pose(axis, 3), 'f', 9));
  for (double angle : principalEulerDegrees(pose.block<3, 3>(0, 0))) fields.append(QString::number(angle, 'f', 9));
  return fields.join(QLatin1Char(','));
}

class CalibrationJobSampler
{
public:
  // The saved motion outlives this worker-local sampler.
  explicit CalibrationJobSampler(const ProbeTouchMotion& motion)
    : m_motion(motion), m_kinematics(motion.robot().model) {}

  ProbeCalibrationRsiResult run()
  {
    const auto steps = probeMotionSteps(m_motion.plan());
    const auto& phases = m_motion.timed().phases();
    if (steps.size() != phases.size())
      return {{}, QStringLiteral("The calibration path has %1 phases for %2 motion steps. Generate it again.")
                      .arg(phases.size()).arg(steps.size())};
    if (steps.size() > StepLimit)
      return {{}, QStringLiteral("%1 steps exceed the %2 steps of RSI_Probe_Calibration.dat.")
                      .arg(steps.size()).arg(StepLimit)};
    const auto tool = wirePose(m_motion.robot().flangeToTcp);
    if (!tool) return {{}, QStringLiteral("Cannot represent the measuring flange-to-TCP transform at six decimal places.")};
    m_job.toolCoordinates = *tool;
    m_job.diagnosticId = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    m_job.streams.append(QVector<V6d>{}); // Touch numbers start at 1.
    for (qsizetype index = 0; index < steps.size(); ++index) {
      const QString error = appendStep(steps[index], phases[index]);
      if (!error.isEmpty()) return {{}, steps[index].phase.text(m_motion.plan()) + QStringLiteral(": ") + error};
    }
    m_job.touchCount = static_cast<int>(m_job.streams.size() - 1);
    const QString planError = writePlan();
    if (!planError.isEmpty()) return {{}, planError};
    return {std::move(m_job), {}};
  }

private:
  QString appendStep(const ProbeMotionStep& step, const MotionPhaseRange& range)
  {
    const auto& points = m_motion.points();
    const ProbeSegment segment = step.phase.segment;
    if (segment == ProbeSegment::ReturnHome) {
      m_job.steps.append(ProbeRsiStep{ProbeRsiStepKind::Home});
      return {};
    }
    if (segment == ProbeSegment::Approach) {
      appendSearch(step.phase, points[range.first]);
      return {};
    }
    const ProbeRsiStepKind kind = segment == ProbeSegment::ToEntry ? ProbeRsiStepKind::Ptp
                                  : segment == ProbeSegment::Transfer ? ProbeRsiStepKind::Circ
                                                                      : ProbeRsiStepKind::Lin;
    auto exported = poseStep(kind, points[range.last]);
    if (!exported) return QStringLiteral("the end pose cannot be represented at six decimal places.");
    if (kind == ProbeRsiStepKind::Circ) {
      const auto auxiliary = arcMidpoint(points[range.first], points[range.last]);
      if (!auxiliary) return QStringLiteral("no CIRC auxiliary point (arc of 180 degrees or rounding).");
      exported->auxiliary = *auxiliary;
    }
    m_job.steps.append(*exported);
    return {};
  }

  // The touch's stream, its Search step and its row of the plan file.
  void appendSearch(const ProbeMotionPhase& phase, const ChamferJointPoint& start)
  {
    const ProbeTouchPlan& plan = m_motion.plan();
    const ProbeTouchPass& pass = plan.passes[phase.pass];
    const ProbeTouch& touch = pass.touches[phase.touch - 1];
    const int number = static_cast<int>(m_job.streams.size());
    m_job.streams.append(searchStream(touch.normal, plan.parameters.searchFeed));
    m_job.steps.append(ProbeRsiStep{ProbeRsiStepKind::Search, number});
    const auto configuration = m_kinematics.configuration(start.joints, m_kinematics.solveFK(start.joints));
    const QString purpose = pass.purpose == ProbeTouchPurpose::Calibration ? QStringLiteral("calibration")
                                                                            : QStringLiteral("verification");
    m_planRows += QStringList{QString::number(number), QString::number(m_job.steps.size()), purpose,
                              QString::number(pass.group), QString::number(pass.repetition),
                              QString::number(phase.touch), QString::number(touch.tilt, 'f', 6),
                              QString::number(touch.roll, 'f', 6), QString::number(touch.normal.x(), 'f', 9),
                              QString::number(touch.normal.y(), 'f', 9), QString::number(touch.normal.z(), 'f', 9),
                              csvPose(touch.start), csvPose(touch.contact), QString::number(configuration.status),
                              QString::number(configuration.turn)}.join(QLatin1Char(','))
                  + QLatin1Char('\n');
  }

  std::optional<ProbeRsiStep> poseStep(ProbeRsiStepKind kind, const ChamferJointPoint& point) const
  {
    const auto coordinates = wirePose(point.tcp);
    if (!coordinates) return std::nullopt;
    const auto configuration = m_kinematics.configuration(point.joints, m_kinematics.solveFK(point.joints));
    return ProbeRsiStep{kind, 0, *coordinates, V6d::Zero(), configuration.status, configuration.turn};
  }

  // Start-shell point halfway along the transfer arc about the sphere centre, with the target's
  // orientation; the plan's arcs are shorter than 180 degrees.
  std::optional<V6d> arcMidpoint(const ChamferJointPoint& start, const ChamferJointPoint& end) const
  {
    const auto& parameters = m_motion.plan().parameters;
    const V3d& centre = parameters.sphereCentre;
    const V3d sum = (start.tcp.block<3, 1>(0, 3) - centre).normalized()
                    + (end.tcp.block<3, 1>(0, 3) - centre).normalized();
    if (!(sum.norm() > 1e-6)) return std::nullopt;
    const double radius = parameters.contactDistance() + ProbeApproachGap;
    return wirePose(makeTransform(end.tcp.block<3, 3>(0, 0), centre + radius * sum.normalized()));
  }

  QString writePlan() const
  {
    const QDir records(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("records")));
    QSaveFile file(records.filePath(QStringLiteral("rsi_calibration_plan_%1.csv").arg(m_job.diagnosticId)));
    const QByteArray contents = (QStringLiteral("# plan_id,") + m_job.diagnosticId
        + QStringLiteral("\n# XYZ mm; ABC principal Z-Y-X deg; tilt and roll deg; search_step: 1-based step in "
                         "calibration_data.txt; start_S/start_T: configuration at the search start\n"
                         "touch,search_step,purpose,group,repetition,pass_touch,tilt,roll,normal_X,normal_Y,normal_Z,"
                         "start_X,start_Y,start_Z,start_A,start_B,start_C,"
                         "contact_X,contact_Y,contact_Z,contact_A,contact_B,contact_C,start_S,start_T\n")
        + m_planRows).toUtf8();
    if (!QDir().mkpath(records.absolutePath()) || !file.open(QIODevice::WriteOnly)
        || file.write(contents) != contents.size() || !file.commit())
      return QStringLiteral("Cannot write the calibration plan %1 (%2).").arg(file.fileName(), file.errorString());
    return {};
  }

  const ProbeTouchMotion& m_motion;
  Kr10Kinematics m_kinematics;
  ProbeCalibrationRsi m_job;
  QString m_planRows;
};

} // namespace

ProbeCalibrationRsiResult sampleProbeCalibrationRsi(const ProbeTouchMotion& motion)
{
  return CalibrationJobSampler(motion).run();
}
