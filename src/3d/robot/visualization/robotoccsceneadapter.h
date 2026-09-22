#pragma once

#include "3d/occt/cadloadworker.h"
#include "3d/occt/occscene.h"

namespace RoboCrap3D {

class RobotOccSceneAdapter final
{
public:
  explicit RobotOccSceneAdapter(OccScene& scene) : m_scene(scene) {}

  bool load(const Kr10Model& model, const CadLoadResult& shapes);
  bool applyTransforms(const std::array<M4d, LinkCount>& transforms);

private:
  void removeRobotParts();

  OccScene& m_scene;
  std::array<OccScene::PartId, LinkCount> m_linkIds{};
  std::optional<OccScene::PartId> m_effPartId;
  std::size_t m_linkCount = 0;
  bool m_loaded = false;
};

} // namespace RoboCrap3D
