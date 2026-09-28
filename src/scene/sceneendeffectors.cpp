#include "sceneendeffectors.h"
#include "geometry/utils.h"

#include <QFileInfo>

#include <cmath>

namespace {

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

bool isLocalCadSource(const QUrl& source)
{
  // File readability and CAD decoding are the loader's responsibility.
  return source.isValid() && source.isLocalFile() && !source.hasQuery()
      && !source.hasFragment() && QFileInfo(source.toLocalFile()).isAbsolute();
}

} // namespace

SceneEndEffectors::SceneEndEffectors(QObject* parent) : QObject(parent)
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
