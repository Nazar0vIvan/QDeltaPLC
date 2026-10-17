#include "occrobotadapter.h"

#include <QStringList>
#include <Standard_Failure.hxx>

namespace RoboCrap3D {

namespace {

constexpr qsizetype ToolCount = 2; // RobotTool::Measuring and RobotTool::Spindle.

gp_Trsf toOccTransform(const M4d& transform)
{
  gp_Trsf result;
  result.SetValues(
      transform(0, 0), transform(0, 1), transform(0, 2), transform(0, 3),
      transform(1, 0), transform(1, 1), transform(1, 2), transform(1, 3),
      transform(2, 0), transform(2, 1), transform(2, 2), transform(2, 3));
  return result;
}

} // namespace

ViewportOperationResult OccRobotAdapter::load(const RobotVisualModel& visuals, const CadLoadResult& shapes, const DrawingPose& pose)
{
  if (m_loaded) return {true, {}};
  const qsizetype linkCount = visuals.links.size();
  if (!m_scene.isValid() || visuals.links.isEmpty() || !m_partIds.isEmpty()
      || visuals.tools.size() != ToolCount || shapes.shapes.size() != linkCount + ToolCount)
    return {false, QStringLiteral("Robot installation: invalid graphics or unfinished installation.")};

  QString operation;
  QString error;
  try {
    for (qsizetype i = 0; i < shapes.shapes.size(); ++i) {
      const bool link = i < linkCount;
      operation = link ? QStringLiteral("Robot link %1 creation").arg(i + 1)
                       : QStringLiteral("Robot tool %1 creation").arg(i - linkCount + 1);
      OccPartProps props;
      if (link) {
        const auto& color = visuals.links[i].color;
        props.color = rgb(color[0], color[1], color[2]);
      }
      const auto id = m_scene.createShapePart(shapes.shapes[i], props);
      if (!id) { error = operation + QStringLiteral(" failed."); break; }
      m_partIds.append(*id);
      // Tools stay hidden until the HOME transforms place them on the flange.
      if (link && !m_scene.setPartVisible(*id, true)) {
        error = operation + QStringLiteral(" display failed.");
        break;
      }
    }
    if (error.isEmpty()) {
      operation = QStringLiteral("Robot initial HOME transforms");
      // Readiness is published only after this entire installation returns success.
      m_loaded = true;
      const auto applied = applyTransforms(pose);
      if (!applied.accepted) error = operation + QStringLiteral(": ") + applied.error;
      operation = QStringLiteral("Robot initial tool display");
      if (error.isEmpty() && !showActiveTool()) error = operation + QStringLiteral(" failed.");
    }
  } catch (const Standard_Failure& failure) {
    error = operation + QStringLiteral(": ") + QString::fromUtf8(failure.what());
  }
  if (error.isEmpty()) return {true, {}};
  if (const QString cleanup = clear(); !cleanup.isEmpty())
    error += QStringLiteral("; Robot installation cleanup: ") + cleanup;
  return {false, error};
}

QString OccRobotAdapter::clear()
{
  m_loaded = false;
  QStringList errors;
  try {
    synchronizeSpindleFrames();
  } catch (const Standard_Failure& failure) {
    errors.append(QStringLiteral("axes: ") + QString::fromUtf8(failure.what()));
  }
  for (qsizetype i = m_partIds.size(); i > 0; --i) {
    const auto id = m_partIds[i - 1];
    try {
      if (m_scene.removePart(id)) m_partIds.removeAt(i - 1);
      else errors.append(QStringLiteral("cannot remove part %1").arg(id));
    } catch (const Standard_Failure& failure) {
      errors.append(QStringLiteral("part %1: %2").arg(id).arg(QString::fromUtf8(failure.what())));
    }
  }
  return errors.join(QStringLiteral("; "));
}

ViewportOperationResult OccRobotAdapter::applyTransforms(const DrawingPose& pose)
{
  const qsizetype linkCount = pose.links.size();
  if (linkCount == 0 || m_partIds.size() != linkCount + ToolCount)
    return {false, QStringLiteral("Robot transform count does not match installed links.")};
  QString operation = QStringLiteral("link transform");
  qsizetype link = 0;
  try {
    for (; link < linkCount; ++link) {
      if (!m_scene.setPartTransform(m_partIds[link], toOccTransform(pose.links[link])))
        return {false, QStringLiteral("link %1 transform rejected").arg(link + 1)};
    }
    operation = QStringLiteral("tool transform");
    for (qsizetype i = linkCount; i < m_partIds.size(); ++i)
      if (!m_scene.setPartTransform(m_partIds[i], toOccTransform(pose.tool)))
        return {false, operation + QStringLiteral(" rejected")};
    m_spindleTcpFrame = pose.tcp;
    m_spindleColletFrame = pose.collet;
    operation = QStringLiteral("tool axes");
    synchronizeSpindleFrames();
  } catch (const Standard_Failure& failure) {
    if (link < linkCount) operation += QStringLiteral(" %1").arg(link + 1);
    return {false, operation + QStringLiteral(": ") + QString::fromUtf8(failure.what())};
  }
  return {true, {}};
}

void OccRobotAdapter::setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet)
{
  m_spindleTcpFrame = tcp;
  m_spindleColletFrame = collet;
  synchronizeSpindleFrames();
}

void OccRobotAdapter::synchronizeSpindleFrames()
{
  m_scene.setSpindleTcpFrame(m_loaded && m_spindleTcpFrame
      ? std::optional<gp_Trsf>{toOccTransform(*m_spindleTcpFrame)} : std::nullopt);
  m_scene.setSpindleColletFrame(m_loaded && m_spindleColletFrame
      ? std::optional<gp_Trsf>{toOccTransform(*m_spindleColletFrame)} : std::nullopt);
}

bool OccRobotAdapter::setActiveTool(RobotTool active)
{
  if (active != RobotTool::Measuring && active != RobotTool::Spindle) return false;
  m_activeTool = active;
  return showActiveTool();
}

bool OccRobotAdapter::showActiveTool()
{
  if (!m_loaded) return true; // Installation displays the active tool after HOME.
  bool success = true;
  const qsizetype firstTool = m_partIds.size() - ToolCount;
  for (qsizetype i = 0; i < ToolCount; ++i)
    success = m_scene.setPartVisible(m_partIds[firstTool + i],
        i == static_cast<qsizetype>(m_activeTool)) && success;
  return success;
}

} // namespace RoboCrap3D
