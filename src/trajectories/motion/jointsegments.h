#pragma once

#include "trajectories/timedmotion.h"

#include <QString>
#include <QVector>
#include <algorithm>
#include <array>
#include <optional>

// Internal PTP/LIN/CIRC/joint-spline operations shared by the motion builder and the motions.

constexpr double TransferPositionTolerance = 0.01; // mm; prescribed TCP chord criterion only.
// mm/s, $VEL_MA.CP of the installed KR C4 (resources/files/KR C4/R1/Mada/$machine.dat).
constexpr double MaximumTransferFeed = 3000.0;

bool rigid(const M4d& transform);
bool withinLimits(const V6d& joints, const RoboCrap3D::Kr10KinematicModel& model);

struct RobotSetupResult
{
  std::optional<ChamferRobotSetup> robot;
  QString error;
};

// The setup with the model's rated joint speeds after the model, timing and $VEL_MA.CP/$ACC_MA.CP
// checks shared by the generated motions.
RobotSetupResult ratedRobotSetup(const ChamferRobotSetup& robot);

// Quintic smoothstep s(u) = 10u^3 - 15u^4 + 6u^5 and its inverse.
double smoothProgress(double u);
double inverseProgress(double progress);

// Synchronized PTP duration for the joint difference: rated joint speeds and configured
// joint accelerations, both scaled by auxiliaryScale.
double ptpDuration(const V6d& delta, const ChamferTimingParameters& timing);

// $VEL.CP/$ACC.CP-style limits of one straight TCP transfer.
struct LinearLimits
{
  double feed = 0.0; // mm/s
  double acceleration = 0.0; // mm/s^2, same acceleration and deceleration; 0 means constant feed.
};

// Timed straight LIN, or CIRC on the arc around centre when angle is nonzero.
struct LinearProfile
{
  M4d first = M4d::Identity();
  M4d last = M4d::Identity();
  Eigen::Quaterniond firstRotation = Eigen::Quaterniond::Identity();
  Eigen::Quaterniond lastRotation = Eigen::Quaterniond::Identity();
  LinearLimits limits;
  V3d centre = V3d::Zero(); // Arc centre.
  V3d axis = V3d::Zero(); // Unit arc axis, from first towards last by the right-hand rule.
  double angle = 0.0; // rad; 0 for a straight LIN.
  double length = 0.0; // Path length, mm.
  double peak = 0.0;
  double ramp = 0.0;
  double cruise = 0.0;

  double duration() const { return 2.0 * ramp + cruise; }
  double progress(double seconds) const
  {
    if (seconds <= 0.0) return 0.0;
    if (seconds >= duration()) return 1.0;
    const double rampDistance = 0.5 * peak * ramp;
    if (seconds < ramp) {
      const double fraction = seconds / ramp;
      return rampDistance * fraction * fraction / length;
    }
    if (seconds < ramp + cruise)
      return (rampDistance + peak * (seconds - ramp)) / length;
    const double remaining = (duration() - seconds) / ramp;
    return std::clamp(1.0 - rampDistance * remaining * remaining / length, 0.0, 1.0);
  }

  M4d target(double seconds) const
  {
    const double u = progress(seconds);
    if (u == 0.0) return first;
    if (u == 1.0) return last;
    M4d pose = first;
    pose.block<3, 1>(0, 3) = angle == 0.0
        ? V3d((1.0 - u) * first.block<3, 1>(0, 3) + u * last.block<3, 1>(0, 3))
        : V3d(centre + Eigen::AngleAxisd(u * angle, axis) * V3d(first.block<3, 1>(0, 3) - centre));
    // Preview rotation follows traveled distance, including both translational ramps.
    pose.block<3, 3>(0, 0) = firstRotation.slerp(u, lastRotation).normalized().toRotationMatrix();
    return pose;
  }
};

// Trapezoidal speed profile, triangular when the ramps meet, constant without acceleration;
// empty for invalid poses or limits.
std::optional<LinearProfile> linearProfile(const M4d& first, const M4d& last, const LinearLimits& limits);
// The CIRC between the end poses of chord around centre, with the limits of chord; empty when the
// end radii differ by more than TransferPositionTolerance or the end directions are (anti)parallel.
std::optional<LinearProfile> circularProfile(const LinearProfile& chord, const V3d& centre);

struct LinearSchedule
{
  QVector<double> localTimes; // Seconds from the path start, one per point after the start.
  QString error;
};

// Fixes every LIN/CIRC point time before IK from the chord criterion of each constant-acceleration span.
LinearSchedule linearSchedule(const LinearProfile& profile);

struct JointSplineResult
{
  QVector<std::array<V6d, 4>> coefficients; // One cubic per interval of the supplied points.
  QString error;
};

// Clamped cubic joint spline with zero end velocities through timed points of one range,
// with joint position and speed checks of every interval.
JointSplineResult clampedJointSpline(const QVector<ChamferJointPoint>& points, const ChamferRobotSetup& robot);
