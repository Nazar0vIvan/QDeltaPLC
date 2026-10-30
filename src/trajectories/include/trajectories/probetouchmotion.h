#pragma once

#include "trajectories/probetouchplan.h"
#include "trajectories/timedmotion.h"

#include <QString>
#include <optional>
#include <utility>

enum class ProbeSegment { ToEntry, Descend, Transfer, Approach, Retract, Unwind, Retreat, ReturnHome };

struct ProbeMotionPhase
{
  int touch = 0; // 1-based touch of Transfer, Approach, Retract and Unwind; 0 otherwise.
  ProbeSegment segment = ProbeSegment::ToEntry;

  // "Entry: descend", "Touch 5/31: approach", "Exit: return HOME".
  QString text() const;
};

// Smallest ball clearances of a probe motion, mm.
struct ProbeClearances
{
  double transfer = 0.0; // To the sphere, outside Approach and Retract.
  double strut = 0.0; // To the strut, at every point.
};

struct ProbeTouchMotionResult;

// Timed animation of the probe touch plan, from HOME back to HOME.
class ProbeTouchMotion
{
public:
  static ProbeTouchMotionResult create(const ProbeTouchPlan& plan, const ChamferRobotSetup& robot);
  const QVector<ChamferJointPoint>& points() const { return m_timed.points(); }
  const ChamferRobotSetup& robot() const { return m_timed.robot(); }
  double duration() const { return m_timed.duration(); }
  double minimumTransferClearance() const { return m_clearances.transfer; }
  double minimumStrutClearance() const { return m_clearances.strut; }
  // One playback segment and one phase per motion step, labelled with ProbeMotionPhase::text().
  const TimedMotion& timed() const { return m_timed; }

private:
  ProbeTouchMotion(TimedMotion timed, const ProbeClearances& clearances)
    : m_timed(std::move(timed)), m_clearances(clearances) {}
  TimedMotion m_timed;
  ProbeClearances m_clearances;
};

struct ProbeTouchMotionResult
{
  std::optional<ProbeTouchMotion> motion;
  QString error;
};
