#pragma once

#include "viewport/occcontroller.h"
#include "cadloadresult.h"

#include <QPointer>
#include <QWindow>
#include <memory>

namespace RoboCrap3D {

class OccViewport;

class OccViewWindow final : public QWindow
{
  Q_OBJECT

public:
  // Borrows the controller's accepted document and pose; replays both on surface recreation.
  OccViewWindow(const DrawingDocument& document, const DrawingPose& pose);
  ~OccViewWindow() override;

  // The OCCT viewer while a native surface exists, otherwise null. Never retain it.
  OccViewport* viewport() const { return m_viewport.get(); }
  // Graphics and parsed shapes define one restorable robot presentation.
  void setScene(const RobotVisualModel& visuals, std::shared_ptr<const CadLoadResult> shapes);
  ViewportOperationResult applyTransforms(const DrawingPose& pose);
  void setActiveTool(RobotTool active);
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
  void attachActiveTool();
  void resizeViewport();
  QPoint nativePosition(const QPointF& position) const;

  std::optional<RobotVisualModel> m_visuals;
  const DrawingDocument& m_document;
  const DrawingPose& m_pose;
  std::shared_ptr<const CadLoadResult> m_shapes;
  RobotTool m_activeTool = RobotTool::Measuring;
  std::unique_ptr<OccViewport> m_viewport;
  bool m_presentationFailed = false;
};

} // namespace RoboCrap3D
