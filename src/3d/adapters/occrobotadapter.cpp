#include "occrobotadapter.h"

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

bool OccRobotAdapter::load(const RobotVisualModel& visuals, const CadLoadResult& shapes,
                                const std::array<M4d, LinkCount>& transforms)
{
  clear();
  if (!m_scene.isValid() || shapes.shapes.size() != static_cast<qsizetype>(LinkCount)) return false;
  for (std::size_t i = 0; i < LinkCount; ++i) {
    const auto id = m_scene.addShapePartWithId(shapes.shapes[i], visuals.links[i].props);
    if (!id) {
      clear();
      return false;
    }
    m_linkIds[i] = *id;
    ++m_linkCount;
  }
  if (!applyTransforms(transforms)) {
    clear();
    return false;
  }
  m_loaded = true;
  for (std::size_t i = 0; i < m_tools.size(); ++i)
    if (m_tools[i].id) (void)m_scene.setPartVisible(*m_tools[i].id, i == static_cast<std::size_t>(m_activeTool));
  return true;
}

void OccRobotAdapter::clear()
{
  m_loaded = false;
  for (const ToolPart& tool : m_tools)
    if (tool.id) (void)m_scene.setPartVisible(*tool.id, false);
  while (m_linkCount > 0) {
    (void)m_scene.removePart(m_linkIds[m_linkCount - 1]);
    --m_linkCount;
  }
}

bool OccRobotAdapter::applyTransforms(const std::array<M4d, LinkCount>& transforms)
{
  if (m_linkCount != LinkCount) return false;
  for (std::size_t i = 0; i < LinkCount; ++i) {
    if (!m_scene.setPartTransform(m_linkIds[i], toOccTransform(transforms[i]))) return false;
  }
  for (const ToolPart& tool : m_tools)
    if (tool.id && !m_scene.setPartTransform(*tool.id, toOccTransform(transforms.back()))) return false;
  m_flange = transforms.back();
  return true;
}

bool OccRobotAdapter::setEndEffectors(
    const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes, SceneEndEffectors::Tool active)
{
  if (active != SceneEndEffectors::Measuring && active != SceneEndEffectors::Spindle) return false;
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
    props.transform = toOccTransform(m_flange);
    const auto replacement = m_scene.addShapePartWithId(shapes[i]->shapes.front(), props);
    if (!replacement) { success = false; continue; }
    if (tool.id) (void)m_scene.removePart(*tool.id);
    tool.id = replacement;
    tool.shapes = shapes[i];
  }
  m_activeTool = active;
  for (std::size_t i = 0; i < m_tools.size(); ++i) {
    if (!m_tools[i].id) continue;
    success = m_scene.setPartVisible(*m_tools[i].id,
        m_loaded && i == static_cast<std::size_t>(active)) && success;
  }
  return success;
}

} // namespace RoboCrap3D
