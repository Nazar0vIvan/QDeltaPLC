#include "sceneendeffectors.h"
#include "geometry/pose.h"
#include "geometry/utils.h"

#include <QFileInfo>

#include <cmath>

namespace {

std::optional<M4d> spindleColletTransform()
{
  const auto y = normalize(V3d{0.999995, -0.000312, -0.003129});
  if (!y) return std::nullopt;
  // Provisional roll: ER Z is the flange's positive Z projected perpendicular to ER Y.
  const auto z = prjUnitOnPlane(V3d::UnitZ(), *y);
  if (!z) return std::nullopt;
  const auto frame = Pose::fromAxes(y->cross(*z), *y, *z,
                                   V3d{142.187099, -0.182323, 122.895995});
  return frame ? std::optional<M4d>{frame->transform()} : std::nullopt;
}

std::optional<M4d> poseTransform(const QList<double>& pose)
{
  if (pose.size() != 6) return std::nullopt;
  for (double value : pose)
    if (!std::isfinite(value)) return std::nullopt;
  const M4d transform = makeTransform(euler2rot(pose[3], pose[4], pose[5]),
                                      V3d{pose[0], pose[1], pose[2]});
  return transform.allFinite() ? std::optional<M4d>{transform} : std::nullopt;
}

QList<double> transformPose(const M4d& transform)
{
  if (!transform.allFinite()) return {};
  const EulerSolution angles = rot2euler(transform.block<3, 3>(0, 0));
  const QList<double> pose{transform(0, 3), transform(1, 3), transform(2, 3),
                          angles.A1, angles.B1, angles.C1};
  for (double value : pose)
    if (!std::isfinite(value)) return {};
  return pose;
}

bool matchesTransform(const std::optional<M4d>& current, const M4d& candidate)
{
  return current && (candidate - *current).cwiseAbs().maxCoeff() <= GeomConst::Eps;
}

bool isLocalCadSource(const QUrl& source)
{
  // File readability and CAD decoding are the loader's responsibility.
  return source.isValid() && source.isLocalFile() && !source.hasQuery()
      && !source.hasFragment() && QFileInfo(source.toLocalFile()).isAbsolute();
}

} // namespace

SceneEndEffectors::SceneEndEffectors(QObject* parent)
    : QObject(parent), m_spindleColletFrame(spindleColletTransform())
{}

bool SceneEndEffectors::setMeasuringCadSource(const QUrl& source)
{
  if (!isLocalCadSource(source)) return false;
  if (m_measuringCadSource == source) return true;
  m_measuringCadSource = source;
  emit measuringCadSourceChanged();
  return true;
}

bool SceneEndEffectors::setSpindleCadSource(const QUrl& source)
{
  if (!isLocalCadSource(source)) return false;
  if (m_spindleCadSource == source) return true;
  m_spindleCadSource = source;
  emit spindleCadSourceChanged();
  return true;
}

bool SceneEndEffectors::setSpindleTcp(const QList<double>& pose)
{
  if (pose.size() != 6) return false;
  for (double value : pose)
    if (!std::isfinite(value)) return false;
  if (m_spindleTcp == pose) return true;
  m_spindleTcp = pose;
  emit spindleTcpChanged();
  return true;
}

QList<double> SceneEndEffectors::spindleColletPose() const
{
  return m_spindleColletFrame ? transformPose(*m_spindleColletFrame) : QList<double>{};
}

QList<double> SceneEndEffectors::spindleTcpInCollet() const
{
  const auto flangeToTcp = poseTransform(m_spindleTcp);
  if (!m_spindleColletFrame || !flangeToTcp) return {};
  const M4d colletToTcp = inverseRigidTransform(*m_spindleColletFrame) * *flangeToTcp;
  // Normalize the composed basis before Euler extraction, including at pitch +/-90 degrees.
  const auto frame = Pose::fromAxes(colletToTcp.block<3, 1>(0, 0), colletToTcp.block<3, 1>(0, 1),
                                   colletToTcp.block<3, 1>(0, 2), colletToTcp.block<3, 1>(0, 3));
  return frame ? transformPose(frame->transform()) : QList<double>{};
}

bool SceneEndEffectors::setSpindleTcpInCollet(const QList<double>& pose)
{
  return applySpindlePoses(spindleColletPose(), pose);
}

bool SceneEndEffectors::applySpindlePoses(const QList<double>& colletPose, const QList<double>& tcpInCollet)
{
  const auto flangeToCollet = poseTransform(colletPose);
  const auto colletToTcp = poseTransform(tcpInCollet);
  if (!flangeToCollet || !colletToTcp) return false;

  const bool colletChanged = !m_spindleColletFrame
      || (colletPose != spindleColletPose() && !matchesTransform(m_spindleColletFrame, *flangeToCollet));
  if (!colletChanged && tcpInCollet == spindleTcpInCollet()) return true;

  // Keep the exact committed ER matrix when only TCP changes.
  const M4d flangeToTcp = (colletChanged ? *flangeToCollet : *m_spindleColletFrame) * *colletToTcp;
  const QList<double> flangePose = transformPose(flangeToTcp);
  if (flangePose.size() != 6) return false;
  const bool tcpChanged = !matchesTransform(poseTransform(m_spindleTcp), flangeToTcp);
  if (!colletChanged) return tcpChanged ? setSpindleTcp(flangePose) : true;

  // ER changes redefine the TCP conversion; commit both before either notification.
  m_spindleColletFrame = *flangeToCollet;
  if (tcpChanged) m_spindleTcp = flangePose;
  emit spindleColletChanged();
  emit spindleTcpChanged();
  return true;
}

bool SceneEndEffectors::setActiveTool(Tool tool)
{
  if (tool != Measuring && tool != Spindle) return false;
  if (m_activeTool == tool) return true;
  m_activeTool = tool;
  emit activeToolChanged();
  return true;
}

QList<double> SceneEndEffectors::spindlePoseFromFlange(const QList<double>& flange) const
{
  const auto baseToFlange = poseTransform(flange);
  const auto flangeToTcp = poseTransform(m_spindleTcp);
  if (!baseToFlange || !flangeToTcp) return {};
  return transformPose((*baseToFlange * *flangeToTcp).eval());
}

QList<double> SceneEndEffectors::flangePoseFromSpindle(const QList<double>& tcp) const
{
  const auto baseToTcp = poseTransform(tcp);
  const auto flangeToTcp = poseTransform(m_spindleTcp);
  if (!baseToTcp || !flangeToTcp) return {};
  return transformPose((*baseToTcp * inverseRigidTransform(*flangeToTcp)).eval());
}
