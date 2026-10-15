#pragma once

#include "robotgraphics.h"
#include <QUrl>

#include <QObject>
#include <QMap>
#include <variant>
#include <QPointer>
#include <QList>
#include <QWindow>

#include <memory>
#include <array>

namespace RoboCrap3D {

struct DrawingPlane {
  V3d origin = V3d::Zero();
  V3d normal = V3d::UnitZ();
  V3d axisU = V3d::UnitX();
  double width = 0.0;
  double height = 0.0;
};

struct DrawingCylinder {
  V3d origin = V3d::Zero();
  V3d axis = V3d::UnitZ();
  double radius = 0.0;
  double length = 0.0;
};

struct DrawingCurve {
  V3d center = V3d::Zero();
  V3d normal = V3d::UnitZ();
  V3d majorAxis = V3d::UnitX();
  double majorRadius = 0.0;
  double minorRadius = 0.0;
};

struct DrawingStyle {
  std::array<int, 3> color{80, 157, 253};
  double lineWidth = 0.0;
  bool decoration = false;
};

struct DrawingPart {
  std::variant<DrawingPlane, DrawingCylinder, DrawingCurve, QList<V3d>, QList<std::array<V3d, 2>>> geometry;
  DrawingStyle style;
};

struct DrawingDiagnostics {
  QList<V3d> markers;
  QList<std::array<V3d, 2>> normalLines;
};

struct DrawingObject {
  quint32 id = 0;
  QList<DrawingPart> parts;
  bool visible = true;
  DrawingDiagnostics diagnostics;
};

struct DrawingAxis {
  V3d center = V3d::Zero();
  V3d direction = V3d::UnitZ();
  double length = 0.0;
};

struct DrawingDocument {
  QMap<quint32, DrawingObject> objects;
  QList<quint32> selected;
  std::optional<DrawingAxis> axis;
};

struct DrawingPose {
  QList<M4d> links;
  M4d tool = M4d::Identity();
  std::optional<M4d> tcp;
  std::optional<M4d> collet;
};

class CadLoadWorker;
class OccViewWindow;
struct CadLoadResult;

class OccController final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QWindow* viewWindow READ viewWindow NOTIFY viewWindowChanged)
  Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
  Q_PROPERTY(bool showSpindleTcp READ showSpindleTcp WRITE setShowSpindleTcp NOTIFY showSpindleTcpChanged)
  Q_PROPERTY(bool showSpindleCollet READ showSpindleCollet WRITE setShowSpindleCollet NOTIFY showSpindleColletChanged)
  Q_PROPERTY(bool viewportReady READ isViewportReady NOTIFY viewportReadyChanged)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
  Q_PROPERTY(QString warningString READ warningString NOTIFY warningStringChanged)
  Q_PROPERTY(bool measuringCadLoading READ isMeasuringCadLoading NOTIFY endEffectorLoadChanged)
  Q_PROPERTY(bool spindleCadLoading READ isSpindleCadLoading NOTIFY endEffectorLoadChanged)
  Q_PROPERTY(QString measuringCadError READ measuringCadError NOTIFY endEffectorLoadChanged)
  Q_PROPERTY(QString spindleCadError READ spindleCadError NOTIFY endEffectorLoadChanged)

public:
  explicit OccController(QObject* parent = nullptr);
  ~OccController() override;
  Q_DISABLE_COPY_MOVE(OccController)

  QWindow* viewWindow() const;
  bool isReady() const;
  bool isViewportReady() const;
  bool isLoading() const { return m_loading; }
  QString errorString() const { return m_error; }
  QString warningString() const { return m_warning; }
  ViewportOperationResult insertObject(const DrawingObject& object);
  void removeObject(quint32 id);
  void setObjectVisible(quint32 id, bool visible);
  void setObjectDiagnostics(quint32 id, const DrawingDiagnostics& diagnostics);
  void setDraftAxis(const std::optional<DrawingAxis>& axis);
  bool showSpindleTcp() const { return m_showSpindleTcp; }
  void setShowSpindleTcp(bool visible);
  bool showSpindleCollet() const { return m_showSpindleCollet; }
  void setShowSpindleCollet(bool visible);
  bool isMeasuringCadLoading() const;
  bool isSpindleCadLoading() const;
  QString measuringCadError() const;
  QString spindleCadError() const;
  void loadEndEffector(RobotTool tool, const QUrl& source);
  void setActiveTool(RobotTool tool);
  void setSpindleFrames(const std::optional<M4d>& tcp, const std::optional<M4d>& collet);
  Q_INVOKABLE void setSelectedObjects(const QList<quint32>& ids);

  ViewportOperationResult loadRobot(const RobotVisualModel& visuals, const DrawingPose& pose);
  ViewportOperationResult applyRobotTransforms(const DrawingPose& pose);
  Q_INVOKABLE void detachViewWindow(QWindow* window);

signals:
  void showSpindleTcpChanged();
  void showSpindleColletChanged();
  void viewWindowChanged();
  void readyChanged();
  void viewportReadyChanged();
  void loadingChanged();
  void errorStringChanged();
  void warningStringChanged();
  void message(const QString& text, bool error);
  void applicationSelectionRequested(quint32 objectId, bool additive);
  void deleteSelectionRequested();
  void endEffectorLoadChanged();
  void endEffectorLoaded(RobotTool tool);

private:
  void createWindow();
  void synchronizeEndEffectors();
  void onViewWindowDestroyed();
  void finishRobotLoad(quint64 generation);
  void startEndEffectorLoad(RobotTool tool);
  // Tool identity, request generation and source identify an asynchronous result.
  void finishEndEffectorLoad(RobotTool tool, quint64 generation, const QUrl& source);
  void setError(const QString& error);

  struct ToolCadState {
    QUrl source;
    std::shared_ptr<const CadLoadResult> shapes;
    std::unique_ptr<CadLoadWorker> worker;
    QString error;
    quint64 generation = 0;
  };

  std::array<ToolCadState, 2> m_toolCad;
  std::optional<RobotVisualModel> m_visuals;
  DrawingPose m_pose;
  DrawingDocument m_document;
  RobotTool m_activeTool = RobotTool::Measuring;
  std::shared_ptr<const CadLoadResult> m_shapes;
  std::unique_ptr<CadLoadWorker> m_worker;
  QPointer<OccViewWindow> m_window;
  QString m_error;
  QString m_warning;
  quint64 m_generation = 0;
  bool m_loading = false;
  bool m_shuttingDown = false;
  bool m_occtInitialized = false;
  bool m_showSpindleTcp = true;
  bool m_showSpindleCollet = true;
};

} // namespace RoboCrap3D
