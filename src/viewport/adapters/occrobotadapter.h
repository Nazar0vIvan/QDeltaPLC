#pragma once

#include "occt/cadloadresult.h"
#include "viewport/occcontroller.h"
#include "occt/occscene.h"
#include <memory>

namespace RoboCrap3D {

class OccRobotAdapter final
{
public:
  explicit OccRobotAdapter(OccScene& scene) : m_scene(scene) {}

  // Visual properties, loaded shapes and pose transforms define a robot presentation.
	ViewportOperationResult load(const RobotVisualModel& visuals, const CadLoadResult& shapes, const DrawingPose& pose);
  bool isLoaded() const { return m_loaded; }
  ViewportOperationResult applyTransforms(const DrawingPose& pose);
  // BASE-relative drawing frames; absent values hide the corresponding axes.
  void setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet);
	bool setEndEffectors(const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes, RobotTool active);

private:
  QString clear();
  void synchronizeSpindleFrames();

  OccScene& m_scene;
  QList<OccScene::PartId> m_linkIds;
  bool m_loaded = false;
  struct ToolPart {
    std::shared_ptr<const CadLoadResult> shapes;
    std::optional<OccScene::PartId> id;
  };
  std::array<ToolPart, 2> m_tools;
  M4d m_toolFrame = M4d::Identity();
  std::optional<M4d> m_spindleTcpFrame;
  std::optional<M4d> m_spindleColletFrame;
  RobotTool m_activeTool = RobotTool::Measuring;
};

} // namespace RoboCrap3D
