#pragma once

#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QWindow>

#include <memory>

namespace RoboCrap3D {

class CadLoadWorker;
class OccViewWindow;
class RobotPreviewState;
struct CadLoadResult;
struct RobotPose;

class OccController final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QWindow* viewWindow READ viewWindow NOTIFY viewWindowChanged)
  Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
  Q_PROPERTY(QString warningString READ warningString NOTIFY warningStringChanged)
  Q_PROPERTY(QVariantList jointAngles READ jointAngles NOTIFY poseChanged)
  Q_PROPERTY(QVariantList flangePose READ flangePose NOTIFY poseChanged)

public:
  explicit OccController(QObject* parent = nullptr);
  ~OccController() override;
  Q_DISABLE_COPY_MOVE(OccController)

  QWindow* viewWindow() const;
  bool isReady() const;
  bool isLoading() const { return m_loading; }
  QString errorString() const { return m_error; }
  QString warningString() const { return m_warning; }
  QVariantList jointAngles() const;
  QVariantList flangePose() const;

  Q_INVOKABLE void loadRobot();
  Q_INVOKABLE void detachViewWindow(QWindow* window);
  Q_INVOKABLE QVariantList solveFK(const QVariantList& joints);
  Q_INVOKABLE QVariantList solveIK(const QVariantList& flange);

signals:
  void viewWindowChanged();
  void readyChanged();
  void loadingChanged();
  void errorStringChanged();
  void warningStringChanged();
  void poseChanged();
  void message(const QString& text, bool error);

private:
  void createWindow();
  void setError(const QString& error);
  bool applyPose(const RobotPose& pose);
  QVariantList solve(const QVariantList& values, bool inverse);

  std::shared_ptr<RobotPreviewState> m_state;
  std::shared_ptr<const CadLoadResult> m_shapes;
  std::unique_ptr<CadLoadWorker> m_worker;
  QPointer<OccViewWindow> m_window;
  QString m_error;
  QString m_warning;
  quint64 m_generation = 0;
  bool m_loading = false;
  bool m_shuttingDown = false;
  bool m_occtInitialized = false;
};

} // namespace RoboCrap3D
