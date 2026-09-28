#pragma once

#include "3d/occt/cadloadresult.h"
#include "3d/robot/model/kr10model.h"
#include "3d/occt/occscene.h"
#include "scene/sceneendeffectors.h"
#include <memory>

namespace RoboCrap3D {

class OccRobotAdapter final
{
public:
  explicit OccRobotAdapter(OccScene& scene) : m_scene(scene) {}

  // Visual properties, loaded shapes and pose transforms define a robot presentation.
  bool load(const RobotVisualModel& visuals, const CadLoadResult& shapes,
            const std::array<M4d, LinkCount>& transforms);
  bool isLoaded() const { return m_loaded; }
  void clear();
  bool applyTransforms(const std::array<M4d, LinkCount>& transforms);
  bool setEndEffectors(const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes,
                      SceneEndEffectors::Tool active);

private:
  OccScene& m_scene;
  std::array<OccScene::PartId, LinkCount> m_linkIds{};
  std::size_t m_linkCount = 0;
  bool m_loaded = false;
  struct ToolPart {
    std::shared_ptr<const CadLoadResult> shapes;
    std::optional<OccScene::PartId> id;
  };
  std::array<ToolPart, 2> m_tools;
  M4d m_flange = M4d::Identity();
  SceneEndEffectors::Tool m_activeTool = SceneEndEffectors::Measuring;
};

} // namespace RoboCrap3D
