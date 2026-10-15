#pragma once

#include "3d/occcontroller.h"
#include "cadloadresult.h"

#include <QPointer>
#include <QList>
#include <QWindow>
#include <memory>

namespace RoboCrap3D {

class OccViewport;

class OccViewWindow final : public QWindow
{
  Q_OBJECT

public:
  explicit OccViewWindow(const DrawingDocument& document);
  ~OccViewWindow() override;

  // Graphics, parsed shapes and transforms define one restorable robot presentation.
  void setScene(const RobotVisualModel& visuals, std::shared_ptr<const CadLoadResult> shapes,
                const DrawingPose& pose);
  ViewportOperationResult insertObject(const DrawingObject& object);
  void removeObject(quint32 id);
  ViewportOperationResult setObjectVisible(quint32 id, bool visible);
  ViewportOperationResult setObjectDiagnostics(quint32 id, const DrawingDiagnostics& diagnostics);
  void setSelectedObjects(const QList<quint32>& ids);
  ViewportOperationResult setDraftAxis(const std::optional<DrawingAxis>& axis);
  ViewportOperationResult applyTransforms(const DrawingPose& pose);
  // BASE-relative drawing frames; absent values hide the corresponding axes.
  void setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet);
  void setEndEffectors(const std::array<std::shared_ptr<const CadLoadResult>, 2>& shapes,
                      RobotTool active);
  bool isReady() const;
  bool isViewportReady() const;
  void releaseSurface();

signals:
  void readyChanged();
  void viewportReadyChanged();
  void errorOccurred(const QString& error);
  void applicationSelectionRequested(quint32 objectId, bool additive);
  void deleteSelectionRequested();

protected:
  bool event(QEvent* event) override;
  void exposeEvent(QExposeEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private:
  void initializeViewport();
  void attachRobot();
  void attachEndEffectors();
  void resizeViewport();
  QPoint nativePosition(const QPointF& position) const;

  std::optional<RobotVisualModel> m_visuals;
  DrawingPose m_pose;
  const DrawingDocument& m_document;
  std::shared_ptr<const CadLoadResult> m_shapes;
  std::array<std::shared_ptr<const CadLoadResult>, 2> m_toolShapes;
  RobotTool m_activeTool = RobotTool::Measuring;
  std::unique_ptr<OccViewport> m_viewport;
  bool m_presentationFailed = false;
};

} // namespace RoboCrap3D
