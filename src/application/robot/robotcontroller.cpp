#include "robotcontroller.h"

#include "viewport/occcontroller.h"
#include "assetpaths.h"
#include "scene/sceneplayback.h"
#include "geometry/utils.h"

#include <QDir>
#include <QThread>
#include <cmath>
#include <utility>

namespace RoboCrap3D {

RobotController::RobotController(OccController* viewport, SceneEndEffectors* effectors, ScenePlayback* playback)
  : m_viewport(viewport), m_endEffectors(effectors), m_playback(playback)
{
  Q_ASSERT(viewport && effectors && playback);
  Q_ASSERT(viewport->thread() == thread() && effectors->thread() == thread() && playback->thread() == thread());
  connect(viewport, &OccController::readyChanged, this, &RobotController::synchronizeReadiness);
  connect(viewport, &OccController::showSpindleTcpChanged, this, &RobotController::synchronizeTools);
  connect(viewport, &OccController::showSpindleColletChanged, this, &RobotController::synchronizeTools);
  connect(effectors, &SceneEndEffectors::activeToolChanged, this, &RobotController::synchronizeTools);
  connect(effectors, &SceneEndEffectors::spindleTcpChanged, this, &RobotController::synchronizeTools);
  connect(effectors, &SceneEndEffectors::spindleColletChanged, this, &RobotController::synchronizeTools);
  synchronizeTools();
  synchronizeReadiness();
}

RobotController::~RobotController()
{
  m_playback->setRobotAvailable(false);
}

bool RobotController::isReady() const
{
  return m_pose && m_viewport->isReady();
}

void RobotController::setError(const QString& error)
{
  if (m_error == error) return;
  m_error = error;
  if (!error.isEmpty()) emit message(error, true);
}

ViewportOperationResult RobotController::failure(const QString& error)
{
  setError(error);
  return {false, error};
}

void RobotController::synchronizeReadiness()
{
  if (m_initialPose && m_viewport->isReady()) {
    m_pose = std::move(m_initialPose);
    m_initialPose.reset();
    m_playback->setRobotModel(m_model.kinematics);
    if (IgnorePreviewJointPositionLimits)
      emit message(QStringLiteral("Offline diagnostic: A1-A6 joint-position limits are disabled in the robot preview."), false);
  }
  m_playback->setRobotAvailable(isReady());
}

DrawingPose RobotController::drawingPose(const QList<M4d>& links, const std::optional<M4d>& checkedTcp) const
{
  DrawingPose result{links, links.back(), {}, {}};
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

void RobotController::synchronizeTools()
{
  const bool spindle = m_endEffectors->activeTool() == SceneEndEffectors::Spindle;
  m_viewport->setActiveTool(spindle ? RobotTool::Spindle : RobotTool::Measuring);
  if (!m_pose && !m_initialPose) return;
  const auto drawing = drawingPose(m_pose ? *m_pose : *m_initialPose);
  m_viewport->setSpindleFrames(drawing.tcp, drawing.collet);
}

ViewportOperationResult RobotController::loadRobot()
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (m_viewport->isLoading()) return {true, {}};
  if (!m_kinematics) {
    m_kinematics = std::make_unique<Kr10Kinematics>(m_model.kinematics);
    auto initialized = initialize();
    if (!initialized.links) {
      m_kinematics.reset();
      return failure(initialized.error);
    }
    m_initialPose = std::move(initialized.links);
  }
  const QDir cadDirectory(ViewportAssets::applicationAssets().cadDirectory);
  auto visuals = m_model.visuals;
  const QDir linkDirectory(cadDirectory.filePath(QStringLiteral("kr10")));
  for (LinkModel& link : visuals.links) link.fileName = linkDirectory.filePath(link.fileName);
  // Fixed end effectors, converted from STEP at build time, in RobotTool order: WP-500 probe, GDZ-65-800A spindle.
  visuals.tools = {cadDirectory.filePath(QStringLiteral("EE_WP-500.brep")),
                   cadDirectory.filePath(QStringLiteral("EE_GDZ-65-800A.brep"))};
  setError({});
  const auto loaded = m_viewport->loadRobot(
      visuals, drawingPose(m_initialPose ? *m_initialPose : *m_pose));
  return loaded.accepted ? ViewportOperationResult{true, {}} : failure(loaded.error);
}

RobotPoseResult RobotController::initialize()
{
  if (!m_kinematics->isValid()) {
    return {{}, QStringLiteral("Invalid KR10 kinematic dimensions.")};
  }

  const auto home = forward(m_model.kinematics.qHome);
  if (!home.links) return home;

  const M4d expected = makeTransform(
      euler2rot(m_model.kinematics.pHome[3], m_model.kinematics.pHome[4], m_model.kinematics.pHome[5]),
      V3d{m_model.kinematics.pHome[0], m_model.kinematics.pHome[1], m_model.kinematics.pHome[2]});
  const M4d difference = inverseRigidTransform(expected) * home.links->back();

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
  return {QList<M4d>(transforms.begin(), transforms.end()), {}};
}

ViewportOperationResult RobotController::applyPose(const QList<M4d>& links, const std::optional<M4d>& checkedTcp)
{
  const auto displayed = m_viewport->applyRobotTransforms(drawingPose(links, checkedTcp));
  if (!displayed.accepted) return displayed;
  m_pose = links;
  return {true, {}};
}

ViewportOperationResult RobotController::applyPlayback(const V6d& joints, const M4d& flangeToTcp)
{
  Q_ASSERT(QThread::currentThread() == thread());
  if (!isReady()) return failure(QStringLiteral("Robot preview is not ready for playback."));
  const auto calculated = forward(joints);
  if (!calculated.links) return failure(calculated.error);
  const M4d tcp = calculated.links->back() * flangeToTcp;
  if (!tcp.allFinite()) return failure(QStringLiteral("The saved trajectory TCP transform is not finite."));
  return applyPose(*calculated.links, tcp);
}

} // namespace RoboCrap3D
