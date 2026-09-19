#include "robotoccsceneadapter.h"

#include <Standard_Failure.hxx>

namespace RoboCrap3D {

bool RobotOccSceneAdapter::load(const Kr10Model& model, const CadLoadResult& shapes)
{
  if (m_loaded || !m_scene.isValid()) return false;
  try {
    for (std::size_t i = 0; i < LinkCount; ++i) {
      const auto id = m_scene.addShapePartWithId(shapes.links[i], model.links[i].props);
      if (!id) {
        m_scene.clearParts();
        return false;
      }
      m_linkIds[i] = *id;
    }
    if (!shapes.endEffector.IsNull()) {
      m_effPartId = m_scene.addShapePartWithId(shapes.endEffector, model.endEffector.props);
      if (!m_effPartId) {
        m_scene.clearParts();
        return false;
      }
    }
    m_loaded = true;
    return true;
  } catch (const Standard_Failure&) {
    m_scene.clearParts();
    m_effPartId.reset();
    return false;
  }
}

bool RobotOccSceneAdapter::applyTransforms(const std::array<M4d, LinkCount>& transforms)
{
  if (!m_loaded) return false;
  for (std::size_t i = 0; i < LinkCount; ++i) {
    if (!m_scene.setPartTransform(m_linkIds[i], transforms[i])) return false;
  }
  return !m_effPartId || m_scene.setPartTransform(*m_effPartId, transforms[EndEffIdx]);
}

} // namespace RoboCrap3D
