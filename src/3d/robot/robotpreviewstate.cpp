#include "robotpreviewstate.h"

#include "3d/occcontroller.h"
#include "3d/viewportassets.h"
#include "scene/scenemachining.h"
#include "geometry/utils.h"

#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <cmath>
#include <functional>
#include <utility>

namespace RoboCrap3D {

namespace {

QVariantList toList(const V6d& values)
{
  QVariantList result;
  result.reserve(6);
  for (double value : values) result.append(value);
  return result;
}

std::optional<V6d> readValues(const QVariantList& values)
{
  if (values.size() != 6) return std::nullopt;
  V6d result{};
  for (int i = 0; i < 6; ++i) {
    const int type = values[i].metaType().id();
    if (type != QMetaType::Double && type != QMetaType::Float
        && type != QMetaType::Int && type != QMetaType::UInt
        && type != QMetaType::LongLong && type != QMetaType::ULongLong) return std::nullopt;
    bool ok = false;
    result[i] = values[i].toDouble(&ok);
    if (!ok || !std::isfinite(result[i])) return std::nullopt;
  }
  return result;
}

} // namespace

RobotController::RobotController(OccController* viewport, SceneEndEffectors* effectors, SceneMachining* machining)
  : m_viewport(viewport), m_endEffectors(effectors), m_machining(machining)
{
  Q_ASSERT(viewport && effectors && machining);
  Q_ASSERT(viewport->thread() == thread() && effectors->thread() == thread() && machining->thread() == thread());
  connect(this, &RobotController::poseChanged, this, &RobotController::tcpPoseChanged);
  connect(viewport, &OccController::readyChanged, this, &RobotController::synchronizeReadiness);
  connect(viewport, &OccController::loadingChanged, this, &RobotController::loadingChanged);
  connect(viewport, &OccController::errorStringChanged, this, &RobotController::onViewportError);
  connect(viewport, &OccController::showSpindleTcpChanged, this, &RobotController::synchronizeTools);
  connect(viewport, &OccController::showSpindleColletChanged, this, &RobotController::synchronizeTools);
  connect(effectors, &SceneEndEffectors::activeToolChanged, this, &RobotController::synchronizeTools);
  connect(effectors, &SceneEndEffectors::spindleTcpChanged, this, &RobotController::synchronizeTools);
  connect(effectors, &SceneEndEffectors::spindleTcpChanged, this, &RobotController::tcpPoseChanged);
  connect(effectors, &SceneEndEffectors::spindleColletChanged, this, &RobotController::synchronizeTools);
  connect(effectors, &SceneEndEffectors::measuringCadSourceChanged, this,
          std::bind(&RobotController::reloadEndEffector, this, SceneEndEffectors::Measuring));
  connect(effectors, &SceneEndEffectors::spindleCadSourceChanged, this,
          std::bind(&RobotController::reloadEndEffector, this, SceneEndEffectors::Spindle));
  reloadEndEffector(SceneEndEffectors::Measuring);
  reloadEndEffector(SceneEndEffectors::Spindle);
  synchronizeTools();
  synchronizeReadiness();
}

RobotController::~RobotController()
{
  m_machining->setRobotAvailable(false);
}

bool RobotController::isReady() const
{
  return m_pose && m_viewport->isReady();
}

bool RobotController::isLoading() const
{
  return m_viewport->isLoading();
}

QVariantList RobotController::jointAngles() const
{
  return m_pose ? toList(m_pose->joints()) : QVariantList{};
}

QVariantList RobotController::flangePose() const
{
  return m_pose ? toList(m_pose->flange()) : QVariantList{};
}

QList<double> RobotController::tcpPose() const
{
  if (!m_pose) return {};
  QList<double> flange;
  for (double value : m_pose->flange()) flange.append(value);
  return m_endEffectors->spindlePoseFromFlange(flange);
}

void RobotController::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  emit errorStringChanged();
  if (!error.isEmpty()) emit message(error, true);
}

RobotOperationResult RobotController::failure(const QString& error)
{
  setError(error);
  return {false, {}, error};
}

void RobotController::onViewportError()
{
  m_error = m_viewport->errorString();
  emit errorStringChanged();
}

void RobotController::synchronizeReadiness()
{
  if (m_initialPose && m_viewport->isReady()) {
    m_pose = std::move(m_initialPose);
    m_initialPose.reset();
    m_machining->setRobotModel(m_model.kinematics);
    if (IgnorePreviewJointPositionLimits)
      emit message(QStringLiteral("Offline diagnostic: A1-A6 joint-position limits are disabled in the robot preview."), false);
    emit poseChanged();
  }
  m_machining->setRobotAvailable(isReady());
  emit readyChanged();
}

DrawingPose RobotController::drawingPose(const RobotPose& pose, const std::optional<M4d>& checkedTcp) const
{
  DrawingPose result{pose.transforms(), pose.transforms().back(), {}, {}};
  if (m_endEffectors->activeTool() != SceneEndEffectors::Spindle) return result;
  if (m_viewport->showSpindleTcp()) {
    if (checkedTcp) result.tcp = checkedTcp;
    else if (m_endEffectors->spindleTcpFrame())
      result.tcp = (result.tool * *m_endEffectors->spindleTcpFrame()).eval();
  }
  if (m_viewport->showSpindleCollet() && m_endEffectors->spindleColletFrame())
    result.collet = (result.tool * *m_endEffectors->spindleColletFrame()).eval();
  return result;
}

bool RobotController::setActiveTool(SceneEndEffectors::Tool tool)
{
  return m_endEffectors->setActiveTool(tool);
}

void RobotController::synchronizeTools()
{
  const bool spindle = m_endEffectors->activeTool() == SceneEndEffectors::Spindle;
  m_viewport->setActiveTool(spindle ? RobotTool::Spindle : RobotTool::Measuring);
  if (!m_pose && !m_initialPose) return;
  const auto drawing = drawingPose(m_pose ? *m_pose : *m_initialPose);
  m_viewport->setSpindleFrames(drawing.tcp, drawing.collet);
}

void RobotController::reloadEndEffector(SceneEndEffectors::Tool tool)
{
  if (tool != SceneEndEffectors::Measuring && tool != SceneEndEffectors::Spindle) return;
  const bool spindle = tool == SceneEndEffectors::Spindle;
  m_viewport->loadEndEffector(spindle ? RobotTool::Spindle : RobotTool::Measuring,
      spindle ? m_endEffectors->spindleCadSource() : m_endEffectors->measuringCadSource());
}

RobotOperationResult RobotController::loadRobot()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (isLoading()) return {true, {}, {}};
  const ViewportAssets assets = ViewportAssets::applicationAssets();
  if (!m_kinematics) {
    const auto model = RobotDefinition::fromJson(assets.modelFile);
    if (!model)
      return failure(QStringLiteral("Cannot read the robot definition: %1").arg(assets.modelFile));
    m_model = *model;
    m_kinematics = std::make_unique<Kr10Kinematics>(m_model.kinematics);
    auto initialized = initialize();
    if (!initialized.pose) {
      m_kinematics.reset();
      return failure(initialized.error);
    }
    m_initialPose = std::move(initialized.pose);
  }
  auto visuals = m_model.visuals;
  for (LinkModel& link : visuals.links) {
    if (link.fileName.isEmpty() || QFileInfo(link.fileName).fileName() != link.fileName
        || link.fileName.contains('/') || link.fileName.contains('\\'))
      return failure(QStringLiteral("The robot definition contains an invalid CAD filename."));
    link.fileName = QDir(assets.cadDirectory).filePath(link.fileName);
  }
  setError({});
  const auto loaded = m_viewport->loadRobot(
      visuals, drawingPose(m_initialPose ? *m_initialPose : *m_pose));
  return loaded.accepted ? RobotOperationResult{true, {}, {}} : failure(loaded.error);
}

RobotPoseResult RobotController::initialize()
{
  if (!m_kinematics->isValid()) {
    return {{}, QStringLiteral("Invalid KR10 kinematic dimensions.")};
  }

  const auto home = forward(m_model.kinematics.qHome);
  if (!home.pose) return home;

  const M4d expected = makeTransform(
      euler2rot(m_model.kinematics.pHome[3], m_model.kinematics.pHome[4], m_model.kinematics.pHome[5]),
      V3d{m_model.kinematics.pHome[0], m_model.kinematics.pHome[1], m_model.kinematics.pHome[2]});
  const M4d difference = inverseRigidTransform(expected) * home.pose->transforms().back();

  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 4; ++col) {
      const double expectedValue = row == col ? 1.0 : 0.0;
      const double tolerance = col == 3 ? Kr10PosEps : Kr10RotEps;
      if (std::abs(difference(row, col) - expectedValue) > tolerance) {
        return {{}, QStringLiteral("KR10 home pose does not match its joint geometry.")};
      }
    }
  }

  return home;
}

RobotPoseResult RobotController::forward(const V6d& joints) const
{
  for (std::size_t i = 0; i < DofCount; ++i) {
    if (!std::isfinite(joints[i])) {
      return {{}, QStringLiteral("Joint %1 must be finite.").arg(i + 1)};
    }
    if (!IgnorePreviewJointPositionLimits
        && (joints[i] < m_model.kinematics.joints[i].qMin
            || joints[i] > m_model.kinematics.joints[i].qMax)) {
      return {{}, QStringLiteral("Joint %1 must be between %2 and %3 degrees.")
                  .arg(i + 1).arg(m_model.kinematics.joints[i].qMin).arg(m_model.kinematics.joints[i].qMax)};
    }
  }

  const auto transforms = m_kinematics->solveFK(joints);
  for (const M4d& transform : transforms) {
    if (!transform.allFinite()) {
      return {{}, QStringLiteral("Kinematics produced a non-finite transform.")};
    }
  }

  const M4d& flange = transforms.back();
  const V3d origin = flange.block<3, 1>(0, 3);
  const EulerSolution angles = rot2euler(flange.block<3, 3>(0, 0));
  const V6d frame{origin.x(), origin.y(), origin.z(), angles.A1, angles.B1, angles.C1};
  return {RobotPose{m_kinematics->configuration(joints, transforms), frame,
                    QList<M4d>(transforms.begin(), transforms.end())}, {}};
}

RobotPoseResult RobotController::inverse(const V6d& flange) const
{
  const M4d target = makeTransform(euler2rot(flange[3], flange[4], flange[5]),
                                 V3d{flange[0], flange[1], flange[2]});
  const auto solution = m_kinematics->calculateIK(target, m_pose->ikSolution());
  if (!solution) {
    return {{}, QStringLiteral("No IK solution in the current configuration. "
                           "The target may be unreachable, outside joint limits, or singular "
                           "(including the q5 = 0 home wrist position).")};
  }
  auto result = forward(solution->q);
  if (!result.pose) return result;
  if (!Kr10Kinematics::matchesFlange(target, result.pose->transforms().back()))
    return {{}, QStringLiteral("Flange FK reconstruction exceeds 0.0001 mm or 0.000001 rotation-entry tolerance.")};
  return result;
}

RobotOperationResult RobotController::applyPose(const RobotPose& pose, const std::optional<M4d>& checkedTcp)
{
  const auto displayed = m_viewport->applyRobotTransforms(drawingPose(pose, checkedTcp));
  if (!displayed.accepted) return {false, {}, displayed.error};
  m_pose = pose;
  emit poseChanged();
  return {true, {}, {}};
}

RobotOperationResult RobotController::solve(const QVariantList& values, bool inverse)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_machining->playbackActive())
    return failure(QStringLiteral("Stop Dry Run before changing the robot pose."));
  if (!isReady()) return failure(QStringLiteral("The robot preview is not ready."));
  const auto input = readValues(values);
  if (!input) return failure(QStringLiteral("Enter exactly six finite numeric values."));
  const auto calculated = inverse ? this->inverse(*input) : forward(*input);
  if (!calculated.pose) return failure(calculated.error);
  auto result = applyPose(*calculated.pose);
  if (!result.accepted) return result;
  result.values = toList(inverse ? calculated.pose->joints() : calculated.pose->flange());
  setError({});
  return result;
}

RobotOperationResult RobotController::solveFK(const QVariantList& joints)
{
  return solve(joints, false);
}

RobotOperationResult RobotController::solveIK(const QVariantList& flange)
{
  return solve(flange, true);
}

RobotOperationResult RobotController::solveTcpIK(const QVariantList& tcp)
{
  const auto values = readValues(tcp);
  if (!values) return failure(QStringLiteral("Enter exactly six finite numeric TCP values."));
  if (!m_endEffectors->spindleTcpFrame())
    return failure(QStringLiteral("Configure the spindle TCP relative to the flange first."));
  QList<double> target;
  for (double value : *values) target.append(value);
  const auto flange = m_endEffectors->flangePoseFromSpindle(target);
  if (flange.isEmpty())
    return failure(QStringLiteral("The TCP target and tool transform produced an invalid flange pose."));
  QVariantList flangeValues;
  for (double value : flange) flangeValues.append(value);
  return solveIK(flangeValues);
}

RobotOperationResult RobotController::applyPlayback(const V6d& joints, const M4d& flangeToTcp)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!isReady()) return failure(QStringLiteral("Robot preview is not ready for playback."));
  const auto calculated = forward(joints);
  if (!calculated.pose) return failure(calculated.error);
  const M4d tcp = calculated.pose->transforms().back() * flangeToTcp;
  if (!tcp.allFinite()) return failure(QStringLiteral("The saved trajectory TCP transform is not finite."));
  return applyPose(*calculated.pose, tcp);
}

} // namespace RoboCrap3D
