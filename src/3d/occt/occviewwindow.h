#pragma once

#include "3d/robot/robotpreviewstate.h"
#include "cadloadworker.h"

#include <QPointer>
#include <QStringList>
#include <QWindow>
#include <memory>

class SceneModel;

namespace RoboCrap3D {

class OccViewport;

class OccViewWindow final : public QWindow
{
  Q_OBJECT

public:
  OccViewWindow();
  ~OccViewWindow() override;

  void setScene(std::shared_ptr<RobotPreviewState> state, std::shared_ptr<const CadLoadResult> shapes);
  void setApplicationScene(SceneModel* scene);
  void synchronizeApplicationScene();
  void setSelectedObjects(const QStringList& ids);
  void setDiagnosticOverlays(bool showPoints, bool showNormals);
  bool applyPose(const RobotPose& pose);
  bool isReady() const;
  void releaseSurface();

signals:
  void readyChanged();
  void errorOccurred(const QString& error);
  void applicationSelectionRequested(const QString& objectId, bool additive);

protected:
  bool event(QEvent* event) override;
  void exposeEvent(QExposeEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  void initializeViewport();
  QPoint nativePosition(const QPointF& position) const;

  std::shared_ptr<RobotPreviewState> m_state;
  std::shared_ptr<const CadLoadResult> m_shapes;
  QPointer<SceneModel> m_applicationScene;
  std::unique_ptr<OccViewport> m_viewport;
  bool m_initializationFailed = false;
};

} // namespace RoboCrap3D
