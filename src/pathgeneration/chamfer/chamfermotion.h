#pragma once

#include <QJsonObject>

#include "chamfercentralmotion.h"
#include "3d/robot/model/kr10kinematicmodel.h"

struct ChamferTimingParameters
{
  double leadInFeed = 10.0; // Legacy saved/QML API; no longer used for generation.
  double machiningFeed = 5.0;
  double leadOutFeed = 10.0; // Legacy saved/QML API; no longer used for generation.
  // Prescribed Cartesian timing for generated paths.
  double accelerationDistanceFraction = 0.30; // Fraction of each lead length, (0, 1].
  double transferFeed = 10.0; // mm/s, shared by both staging transfers.
  double transferAcceleration = 20.0; // mm/s^2, same acceleration and deceleration.
  double cartesianAcceleration = 20.0; // Existing simulation bound, not controller $ACC_MA.
  // Resolved from the robot model by ChamferMotion::create; retained in saved motion.
  V6d jointSpeed = V6d::Zero(); // deg/s; zero means no model has been applied yet.
  // Simulation acceleration assumptions, not calibrated KUKA controller limits.
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
  V6d velocity = V6d::Zero(); // deg/s; retained HOME derivatives, not playback spline derivatives.
};

struct ChamferPlaybackPose
{
  V6d joints = V6d::Zero();
  M4d tcp = M4d::Identity();
  ChamferMotionPhase phase = ChamferMotionPhase::Approach;
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
  double centralTimeScale() const { return 1.0; } // Compatibility: prescribed timing is never stretched.
  const std::optional<ChamferCentralMotion>& centralMotion() const { return m_centralMotion; }
  // Absolute elapsed seconds. Finite times outside the motion clamp to its ends.
  std::optional<ChamferPlaybackPose> evaluate(double seconds) const;

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
  QJsonObject diagnostics; // Owned worker result, also returned on failure.
};
