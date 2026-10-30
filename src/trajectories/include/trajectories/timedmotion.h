#pragma once

#include "kinematics/kr10kinematicmodel.h"

#include <QString>
#include <QVector>
#include <array>
#include <optional>
#include <utility>

struct ChamferTimingParameters
{
  double machiningFeed = 5.0;
  // Prescribed Cartesian timing for generated paths.
  double accelerationDistanceFraction = 0.30; // Fraction of each lead length, (0, 1].
  double transferFeed = 10.0; // mm/s, shared by both staging transfers; at most $VEL_MA.CP.
  double transferAcceleration = 20.0; // mm/s^2, same acceleration and deceleration; at most cartesianAcceleration.
  // mm/s^2, $ACC_MA.CP of the installed KR C4 (resources/files/KR C4/R1/Mada/$machine.dat):
  // the maximum programmable CP path acceleration. The controller does not monitor it for RSI.
  double cartesianAcceleration = 10000.0;
  // Resolved from the robot model by ratedRobotSetup; retained in saved motion.
  V6d jointSpeed = V6d::Zero(); // deg/s; zero means no model has been applied yet.
  // HOME PTP animation timing only. The controller has no fixed axis
  // acceleration ($ADAP_ACC=#STEP2 dynamic model), so none is enforced.
  V6d jointAcceleration = V6d::Constant(60.0); // deg/s^2
  double auxiliaryScale = 1.0; // (0, 1], scales joint limits for HOME movements.
};

struct ChamferRobotSetup
{
  RoboCrap3D::Kr10KinematicModel model;
  M4d flangeToTcp = M4d::Identity();
  ChamferTimingParameters timing;
};

struct ChamferJointPoint
{
  V6d joints = V6d::Zero(); // Degrees, continuous winding, never modulo 360.
  M4d tcp = M4d::Identity(); // BASE-relative, evaluated through FK.
  double progress = 0.0; // Local geometric phase progress, not time.
  double time = 0.0; // Seconds from HOME departure.
};

// Smoothstep: PTP between the segment's end joints; Spline: stored clamped cubic per interval;
// JointLinear: constant-speed LIN between consecutive points.
enum class MotionInterpolation { Smoothstep, Spline, JointLinear };

// Inclusive point range played with one interpolation; the first point is the preceding
// segment's last point.
struct MotionSegment
{
  qsizetype first = 0;
  qsizetype last = 0;
  MotionInterpolation interpolation = MotionInterpolation::Smoothstep;
  qsizetype spline = -1; // First coefficient interval of a Spline segment.
};

// Inclusive point range shown under one label; the ranges cover all points in order.
struct MotionPhaseRange
{
  qsizetype first = 0;
  qsizetype last = 0;
  QString label;
};

struct TimedMotionData
{
  QVector<ChamferJointPoint> points;
  QVector<MotionSegment> segments;
  QVector<MotionPhaseRange> phases;
  QVector<std::array<V6d, 4>> splines;
};

// Timed joint points of a generated motion; Dry Run plays them without IK, FK or spline fitting.
class TimedMotion
{
public:
  TimedMotion(const ChamferRobotSetup& robot, TimedMotionData data)
    : m_robot(robot), m_data(std::move(data)) {}
  const ChamferRobotSetup& robot() const { return m_robot; }
  const QVector<ChamferJointPoint>& points() const { return m_data.points; }
  const QVector<MotionSegment>& segments() const { return m_data.segments; }
  const QVector<MotionPhaseRange>& phases() const { return m_data.phases; }
  double duration() const { return m_data.points.isEmpty() ? 0.0 : m_data.points.back().time; }
  // Joints and phase index at absolute seconds; finite times outside the motion clamp to its ends.
  // A point shared by two segments or phases belongs to the ending one.
  std::optional<std::pair<V6d, qsizetype>> evaluateJoints(double seconds) const;

private:
  ChamferRobotSetup m_robot;
  TimedMotionData m_data;
};
