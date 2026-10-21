#pragma once

#include <utility>

#include "trajectories/chamfercentralmotion.h"
#include "kinematics/kr10kinematicmodel.h"

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
  // Resolved from the robot model by ChamferMotion::create; retained in saved motion.
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

enum class ChamferMotionPhase { Approach, TransferIn, LeadIn, Machining, LeadOut, TransferOut, ReturnHome };

struct ChamferJointPoint
{
  V6d joints = V6d::Zero(); // Degrees, continuous winding, never modulo 360.
  M4d tcp = M4d::Identity(); // BASE-relative, evaluated through FK.
  double progress = 0.0; // Local geometric phase progress, not time.
  double time = 0.0; // Seconds from HOME departure.
};

struct ChamferMotionResult;

// Timed simulation motion; does not emulate controller-specific KRL interpolation.
class ChamferMotion
{
public:
  static ChamferMotionResult create(const ChamferPath& path, const ChamferRobotSetup& robot);
  const QVector<ChamferJointPoint>& points() const { return m_points; }
  // Inclusive ranges, in ChamferMotionPhase order. Junction points are shared.
  const std::array<qsizetype, 8>& boundaries() const { return m_boundaries; }
  const ChamferRobotSetup& robot() const { return m_robot; }
  const ChamferPathParameters& parameters() const { return m_parameters; }
  double duration() const { return m_points.isEmpty() ? 0.0 : m_points.back().time; }
  const std::optional<ChamferCentralMotion>& centralMotion() const { return m_centralMotion; }
  // Absolute elapsed seconds. Finite times outside the motion clamp to its ends.
  std::optional<std::pair<V6d, ChamferMotionPhase>> evaluateJoints(double seconds) const;

private:
  // Source curve, robot calibration and complete point/range data define a motion.
  ChamferMotion(const ChamferPath& path, const ChamferRobotSetup& robot,
                QVector<ChamferJointPoint> points, const std::array<qsizetype, 8>& boundaries);
  void setCentralMotion(ChamferCentralMotion motion);
  QString preparePlaybackSplines();
  QString prepareJointSpline(qsizetype first, qsizetype last);
  ChamferPathParameters m_parameters;
  ChamferRobotSetup m_robot;
  QVector<ChamferJointPoint> m_points;
  std::array<qsizetype, 8> m_boundaries{};
  std::optional<ChamferCentralMotion> m_centralMotion;
  // Non-HOME intervals, indexed from boundaries[1]; independent clamped LIN/central splines.
  QVector<std::array<V6d, 4>> m_jointSpline;
};

struct ChamferMotionResult
{
  std::optional<ChamferMotion> motion;
  QString error;
};
