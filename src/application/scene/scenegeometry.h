#pragma once

#include "geometry/boundedplane.h"
#include "geometry/boundedcylinder.h"
#include "geometry/surfaceintersection.h"
#include "trajectories/chamfermotion.h"
#include "trajectories/probetouchmotion.h"
#include <memory>
#include <QList>
#include <QObject>
#include <optional>
#include <utility>

// Value-type draft settings; geometry, robot model and tool transform are supplied by C++.
struct SceneMachiningSettings
{
  Q_GADGET
  Q_PROPERTY(double chamferSize MEMBER chamferSize)
  Q_PROPERTY(double chamferAngle MEMBER chamferAngle)
  Q_PROPERTY(double stagingDistance MEMBER stagingDistance)
  Q_PROPERTY(bool flipAxis MEMBER flipAxis)
  Q_PROPERTY(double leadInClearance MEMBER leadInClearance)
  Q_PROPERTY(double leadOutClearance MEMBER leadOutClearance)
  Q_PROPERTY(double leadInSpan MEMBER leadInSpan)
  Q_PROPERTY(double leadOutSpan MEMBER leadOutSpan)
  Q_PROPERTY(double machiningFeed MEMBER machiningFeed)
  Q_PROPERTY(double accelerationDistanceFraction MEMBER accelerationDistanceFraction)
  Q_PROPERTY(double transferFeed MEMBER transferFeed)
  Q_PROPERTY(double transferAcceleration MEMBER transferAcceleration)
  Q_PROPERTY(double acceleration MEMBER acceleration)
  Q_PROPERTY(double auxiliaryScale MEMBER auxiliaryScale)
  Q_PROPERTY(QList<double> jointSpeed MEMBER jointSpeed)
  Q_PROPERTY(QList<double> jointAcceleration MEMBER jointAcceleration)
public:
  double chamferSize = 0.5;
  double chamferAngle = 45.0;
  double stagingDistance = 10.0;
  bool flipAxis = false;
  double leadInClearance = 5.0; // mm
  double leadOutClearance = 5.0; // mm
  double leadInSpan = 30.0; // degrees
  double leadOutSpan = 30.0; // degrees
  double machiningFeed = 5.0;
  double accelerationDistanceFraction = 0.30;
  double transferFeed = 10.0; // mm/s, shared staging-transfer setting.
  double transferAcceleration = 20.0; // mm/s^2, requested transfer setting.
  double acceleration = 10000.0; // mm/s^2, maximum programmable CP path acceleration $ACC_MA.CP.
  double auxiliaryScale = 1.0;
  QList<double> jointSpeed{0, 0, 0, 0, 0, 0}; // Model-derived snapshot; unresolved before model loading.
  QList<double> jointAcceleration{60, 60, 60, 60, 60, 60};

  static SceneMachiningSettings fromMotion(const ChamferMotion& motion);
  ChamferPathParameters applyTo(ChamferPathParameters parameters) const;
  std::optional<ChamferTimingParameters> timing() const;
};

struct SceneMachiningData
{
  std::shared_ptr<const ChamferMotion> motion;
  quint32 sourceEdgeId = 0;
  quint64 robotSetupRevision = 0;
};

class SceneMachiningPath final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double duration READ duration CONSTANT)
  Q_PROPERTY(double centralDuration READ centralDuration CONSTANT)
  Q_PROPERTY(quint32 sourceEdgeId READ sourceEdgeId CONSTANT)
  Q_PROPERTY(SceneMachiningSettings settings READ settings CONSTANT)
  Q_PROPERTY(QString incompatibility READ incompatibility NOTIFY compatibilityChanged)
  Q_PROPERTY(bool compatible READ compatible NOTIFY compatibilityChanged)
public:
  SceneMachiningPath(SceneMachiningData data, QObject* parent)
    : QObject(parent), m_data(std::move(data)) {}
  const ChamferMotion& motion() const { return *m_data.motion; }
  const SceneMachiningData& data() const { return m_data; }
  double duration() const { return motion().duration(); }
  double centralDuration() const
  { return motion().centralMotion() ? motion().centralMotion()->duration() : 0.0; }
  quint32 sourceEdgeId() const { return m_data.sourceEdgeId; }
  SceneMachiningSettings settings() const { return SceneMachiningSettings::fromMotion(motion()); }
  QString incompatibility() const { return m_incompatibility; }
  bool compatible() const { return m_incompatibility.isEmpty(); }
  void setIncompatibility(const QString& reason)
  {
    if (reason == m_incompatibility) return;
    m_incompatibility = reason;
    emit compatibilityChanged();
  }
signals:
  void compatibilityChanged();
private:
  const SceneMachiningData m_data;
  QString m_incompatibility;
};

// Intersection curve with the two source object IDs, smaller ID first.
struct SceneEdge
{
  EdgeGeometry edge;
  std::array<quint32, 2> sources{};
};

// Calibration sphere fitted to the ruby ball centers of the jogged touches.
struct SceneSphere
{
  V3d center = V3d::Zero(); // Fitted center, shared by both fitted spheres.
  double radius = 0.0; // mm, nominal: half the sphere file's diameter.
  double ballCenterRadius = 0.0; // mm, sphere through the ball centers.
  double compensatedRadius = 0.0; // mm, ballCenterRadius minus the ruby ball radius.
  double rms = 0.0; // mm, fit residual.
  QList<V3d> roughPoints; // Ball centers reported by the controller at the jogged touches.
  double strutRadius = 0.0; // mm, vertical strut under the sphere.
  double strutRecess = 0.0; // mm, strut top above the lowest sphere point.

  bool operator==(const SceneSphere& other) const
  {
    return center == other.center && radius == other.radius && ballCenterRadius == other.ballCenterRadius
        && compensatedRadius == other.compensatedRadius && rms == other.rms && roughPoints == other.roughPoints
        && strutRadius == other.strutRadius && strutRecess == other.strutRecess;
  }
};

// Generated probe calibration motion, shared with playback.
struct SceneTouchPath
{
  std::shared_ptr<const ProbeTouchMotion> motion;
};
