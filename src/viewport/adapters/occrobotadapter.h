#pragma once

#include "occt/cadloadresult.h"
#include "viewport/occcontroller.h"
#include "occt/occscene.h"

namespace RoboCrap3D {

class OccRobotAdapter final
{
public:
  explicit OccRobotAdapter(OccScene& scene) : m_scene(scene) {}

  // Visual properties, loaded link/tool shapes and pose transforms define a robot presentation.
	ViewportOperationResult load(const RobotVisualModel& visuals, const CadLoadResult& shapes, const DrawingPose& pose);
  bool isLoaded() const { return m_loaded; }
  ViewportOperationResult applyTransforms(const DrawingPose& pose);
  // BASE-relative drawing frames; absent values hide the corresponding axes.
  void setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet);
  bool setActiveTool(RobotTool active);

private:
  QString clear();
  void synchronizeSpindleFrames();
  bool showActiveTool();

  OccScene& m_scene;
  // Links in model order, then the measuring and spindle tools.
  QList<OccScene::PartId> m_partIds;
  bool m_loaded = false;
  std::optional<M4d> m_spindleTcpFrame;
  std::optional<M4d> m_spindleColletFrame;
  RobotTool m_activeTool = RobotTool::Measuring;
};

} // namespace RoboCrap3D
