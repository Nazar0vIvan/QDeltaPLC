#pragma once

#include "trajectories/chamfermotion.h"
#include "trajectories/probetouchplan.h"

#include <QString>
#include <QVector>
#include <array>
#include <optional>
#include <utility>

enum class ProbeSegment { ToEntry, Descend, Transfer, Approach, Retract, Retreat, ReturnHome };

struct ProbeMotionPhase
{
  int touch = 0; // 1-based touch of Transfer, Approach and Retract; 0 otherwise.
  ProbeSegment segment = ProbeSegment::ToEntry;

  // "Entry: descend", "Touch 5/31: approach", "Exit: return HOME".
  QString text() const;
};

struct ProbeMotionSegment
{
  qsizetype first = 0; // Inclusive point range; the first point is the preceding segment's last.
  qsizetype last = 0;
  qsizetype spline = -1; // First interval in the spline coefficients for Descend/Transfer/Retreat.
  ProbeMotionPhase phase;
};

struct ProbeTouchMotionData
{
  QVector<ChamferJointPoint> points;
  QVector<ProbeMotionSegment> segments;
  QVector<std::array<V6d, 4>> splines;
  double minimumTransferClearance = 0.0; // mm between the ball and the sphere outside touches.
  double minimumStrutClearance = 0.0; // mm between the ball and the strut.
};

struct ProbeTouchMotionResult;

// Timed animation of the probe touch plan, from HOME back to HOME.
class ProbeTouchMotion
{
public:
  static ProbeTouchMotionResult create(const ProbeTouchPlan& plan, const ChamferRobotSetup& robot);
  const QVector<ChamferJointPoint>& points() const { return m_data.points; }
  const QVector<ProbeMotionSegment>& segments() const { return m_data.segments; }
  const ChamferRobotSetup& robot() const { return m_robot; }
  double duration() const { return m_data.points.back().time; }
  double minimumTransferClearance() const { return m_data.minimumTransferClearance; }
  double minimumStrutClearance() const { return m_data.minimumStrutClearance; }
  // Absolute elapsed seconds; finite times outside the motion clamp to its ends.
  std::optional<std::pair<V6d, ProbeMotionPhase>> evaluateJoints(double seconds) const;

private:
  ProbeTouchMotion(const ChamferRobotSetup& robot, ProbeTouchMotionData data)
    : m_robot(robot), m_data(std::move(data)) {}
  ChamferRobotSetup m_robot;
  ProbeTouchMotionData m_data;
};

struct ProbeTouchMotionResult
{
  std::optional<ProbeTouchMotion> motion;
  QString error;
};
