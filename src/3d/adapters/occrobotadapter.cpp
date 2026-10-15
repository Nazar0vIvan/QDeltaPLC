#include "occrobotadapter.h"

#include <QStringList>
#include <Standard_Failure.hxx>

namespace RoboCrap3D {

namespace {

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
  if (!m_scene.isValid() || visuals.links.isEmpty() || !m_linkIds.isEmpty()
      || shapes.shapes.size() != visuals.links.size())
    return {false, QStringLiteral("Robot installation: invalid graphics or unfinished installation.")};

  QString operation;
  QString error;
  try {
    for (qsizetype i = 0; i < visuals.links.size(); ++i) {
      operation = QStringLiteral("Robot link %1 creation").arg(i + 1);
      const auto& color = visuals.links[i].color;
      OccPartProps props;
      props.color = rgb(color[0], color[1], color[2]);
      const auto id = m_scene.createShapePart(shapes.shapes[i], props);
      if (!id) { error = operation + QStringLiteral(" failed."); break; }
      m_linkIds.append(*id);
      if (!m_scene.setPartVisible(*id, true)) {
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
      if (error.isEmpty()) {
        operation = QStringLiteral("Robot initial tool display");
        for (std::size_t i = 0; i < m_tools.size(); ++i)
          if (m_tools[i].id && !m_scene.setPartVisible(*m_tools[i].id, i == static_cast<std::size_t>(m_activeTool))) {
            error = operation + QStringLiteral(" failed.");
            break;
          }
      }
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
    for (const ToolPart& tool : m_tools)
      if (tool.id && !m_scene.setPartVisible(*tool.id, false))
        errors.append(QStringLiteral("cannot hide tool"));
  } catch (const Standard_Failure& failure) {
    errors.append(QStringLiteral("tool/axes: ") + QString::fromUtf8(failure.what()));
  }
  for (qsizetype i = m_linkIds.size(); i > 0; --i) {
    const auto id = m_linkIds[i - 1];
    try {
      if (m_scene.removePart(id)) m_linkIds.removeAt(i - 1);
      else errors.append(QStringLiteral("cannot remove part %1").arg(id));
    } catch (const Standard_Failure& failure) {
      errors.append(QStringLiteral("part %1: %2").arg(id).arg(QString::fromUtf8(failure.what())));
    }
  }
  return errors.join(QStringLiteral("; "));
}

ViewportOperationResult OccRobotAdapter::applyTransforms(const DrawingPose& pose)
{
  if (m_linkIds.isEmpty() || m_linkIds.size() != pose.links.size())
    return {false, QStringLiteral("Robot transform count does not match installed links.")};
  QString operation = QStringLiteral("link transform");
  qsizetype link = 0;
  try {
    for (; link < m_linkIds.size(); ++link) {
      if (!m_scene.setPartTransform(m_linkIds[link], toOccTransform(pose.links[link])))
        return {false, QStringLiteral("link %1 transform rejected").arg(link + 1)};
    }
    operation = QStringLiteral("tool transform");
    for (const ToolPart& tool : m_tools)
      if (tool.id && !m_scene.setPartTransform(*tool.id, toOccTransform(pose.tool)))
        return {false, operation + QStringLiteral(" rejected")};
    m_toolFrame = pose.tool;
    m_spindleTcpFrame = pose.tcp;
    m_spindleColletFrame = pose.collet;
    operation = QStringLiteral("tool axes");
    synchronizeSpindleFrames();
  } catch (const Standard_Failure& failure) {
    if (link < m_linkIds.size()) operation += QStringLiteral(" %1").arg(link + 1);
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

bool OccRobotAdapter::setEndEffectors(
    const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes, RobotTool active)
{
  if (active != RobotTool::Measuring && active != RobotTool::Spindle) return false;
  bool success = true;
  for (std::size_t i = 0; i < m_tools.size(); ++i) {
    ToolPart& tool = m_tools[i];
    if (tool.shapes == shapes[i]) continue;
    if (!shapes[i]) {
      if (tool.id) (void)m_scene.removePart(*tool.id);
      tool = ToolPart{};
      continue;
    }
    if (shapes[i]->shapes.size() != 1) { success = false; continue; }
    OccPartProps props;
    props.transform = toOccTransform(m_toolFrame);
    const auto replacement = m_scene.addShapePartWithId(shapes[i]->shapes.front(), props);
    if (!replacement) { success = false; continue; }
    if (tool.id) (void)m_scene.removePart(*tool.id);
    tool.id = replacement;
    tool.shapes = shapes[i];
  }
  m_activeTool = active;
  synchronizeSpindleFrames();
  for (std::size_t i = 0; i < m_tools.size(); ++i) {
    if (!m_tools[i].id) continue;
    success = m_scene.setPartVisible(*m_tools[i].id,
        m_loaded && i == static_cast<std::size_t>(active)) && success;
  }
  return success;
}

} // namespace RoboCrap3D
