#pragma once

#include "jointsegments.h"
#include "tcpsolver.h"
#include "trajectories/timedmotion.h"

#include <QString>
#include <QVector>
#include <optional>
#include <utility>

// Point times of a Cartesian path, fixed before IK, and its phase labels.
struct PathSchedule
{
  QVector<double> times; // Local seconds of every point after the start, increasing.
  // (Last path-local point index of the phase, label); index 0 is the start, the last phase
  // ends at times.size().
  QVector<std::pair<qsizetype, QString>> phases;
  QString error;
};

// Prescribed TCP pose of one scheduled point and the progress stored with it
// (LIN/CIRC: travelled fraction; curve: geometric phase progress, 1 at a phase end).
struct PathTarget
{
  M4d tcp = M4d::Identity();
  double progress = 0.0;
};

// A timed Cartesian TCP path of one motion segment.
class TimedPath
{
public:
  virtual ~TimedPath() = default;
  virtual PathSchedule schedule() const = 0;
  // Target of the path-local point index at local seconds; index 0 is the start (local 0).
  virtual std::optional<PathTarget> target(qsizetype index, double seconds) const = 0;
};

// LIN or CIRC of a LinearProfile under one phase label.
class LinearPath final : public TimedPath
{
public:
  LinearPath(const LinearProfile& profile, const QString& label) : m_profile(profile), m_label(label) {}
  PathSchedule schedule() const override;
  std::optional<PathTarget> target(qsizetype index, double seconds) const override;

private:
  LinearProfile m_profile;
  QString m_label;
};

// Appends PTP and Cartesian segments from HOME with one IK and check sequence and collects the
// timed points, segments, phases and splines of a TimedMotion. Borrows the worker-owned setup.
class MotionBuilder
{
public:
  explicit MotionBuilder(const ChamferRobotSetup& robot);
  const ChamferTcpSolver& solver() const { return m_solver; }
  // Configuration of the last point, the seed of the next IK.
  const RoboCrap3D::Kr10Kinematics::IkSolution& seed() const { return m_seed; }
  const QVector<ChamferJointPoint>& points() const { return m_data.points; }
  RoboCrap3D::Kr10Kinematics::IkSolution home() const;
  // Smoothstep PTP from the last point to end, which becomes the seed.
  QString appendPtp(const RoboCrap3D::Kr10Kinematics::IkSolution& end, const QString& label);
  // Cartesian path from the last point; nothing is stored on failure.
  QString appendPath(const TimedPath& path, MotionInterpolation interpolation);
  TimedMotion finish() const { return TimedMotion(m_robot, m_data); }

private:
  struct RateIssue
  {
    qsizetype index = 0; // Path-local point of the failure.
    QString message; // Empty when the rates are accepted.
  };
  RateIssue rateIssue(const QVector<ChamferJointPoint>& points, const QVector<V3d>& targets) const;

  const ChamferRobotSetup& m_robot;
  ChamferTcpSolver m_solver;
  TimedMotionData m_data;
  RoboCrap3D::Kr10Kinematics::IkSolution m_seed;
};
