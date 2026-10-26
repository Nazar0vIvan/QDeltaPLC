#include "sceneendeffectors.h"
#include "geometry/utils.h"

#include <cmath>

namespace {

// Normalizes the axis columns; rejects a nonorthogonal basis or a nonfinite origin.
std::optional<M4d> frameTransform(const M3d& axes, const V3d& origin)
{
  if (!origin.allFinite()) return std::nullopt;
  const auto basis = vecs2basis(axes.col(0), axes.col(1), axes.col(2));
  if (!basis) return std::nullopt;
  return makeTransform(basis2rot(*basis), origin);
}

std::optional<M4d> spindleColletTransform()
{
  const auto y = normalize(V3d{0.999995, -0.000312, -0.003129});
  if (!y) return std::nullopt;
  // Provisional roll: ER Z is the flange's positive Z projected perpendicular to ER Y.
  const auto z = prjUnitOnPlane(V3d::UnitZ(), *y);
  if (!z) return std::nullopt;
  M3d axes;
  axes << y->cross(*z), *y, *z;
  return frameTransform(axes, V3d{142.187099, -0.182323, 122.895995});
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

} // namespace

SceneEndEffectors::SceneEndEffectors(QObject* parent)
    : QObject(parent), m_spindleColletFrame(spindleColletTransform()),
      m_measuringTcpFrame(poseTransform(m_measuringTcp))
{}

bool SceneEndEffectors::applyMeasuringTcp(const QList<double>& pose)
{
  if (m_measuringTcpFrame && m_measuringTcp == pose) return true;
  const auto frame = poseTransform(pose);
  if (!frame) return false;
  m_measuringTcp = pose;
  m_measuringTcpFrame = frame;
  emit measuringTcpChanged();
  return true;
}

bool SceneEndEffectors::setSpindleTcp(const QList<double>& pose)
{
  if (m_spindleTcpFrame && m_spindleTcp == pose) return true;
  const auto frame = poseTransform(pose);
  if (!frame) return false;
  m_spindleTcp = pose;
  m_spindleTcpFrame = frame;
  emit spindleTcpChanged();
  return true;
}

QList<double> SceneEndEffectors::spindleColletPose() const
{
  return m_spindleColletFrame ? transformPose(*m_spindleColletFrame) : QList<double>{};
}

QList<double> SceneEndEffectors::spindleTcpInCollet() const
{
  if (!m_spindleColletFrame || !m_spindleTcpFrame) return {};
  const M4d colletToTcp = inverseRigidTransform(*m_spindleColletFrame) * *m_spindleTcpFrame;
  // Normalize the composed basis before Euler extraction, including at pitch +/-90 degrees.
  const auto frame = frameTransform(colletToTcp.block<3, 3>(0, 0), colletToTcp.block<3, 1>(0, 3));
  return frame ? transformPose(*frame) : QList<double>{};
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
  const bool tcpChanged = !matchesTransform(m_spindleTcpFrame, flangeToTcp);
  if (!colletChanged) return tcpChanged ? setSpindleTcp(flangePose) : true;

  const auto acceptedTcpFrame = tcpChanged ? poseTransform(flangePose) : m_spindleTcpFrame;
  if (!acceptedTcpFrame) return false;

  // ER changes redefine the TCP conversion; commit both before either notification.
  m_spindleColletFrame = *flangeToCollet;
  if (tcpChanged) m_spindleTcp = flangePose;
  m_spindleTcpFrame = acceptedTcpFrame;
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
