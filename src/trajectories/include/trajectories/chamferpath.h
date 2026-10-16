#pragma once

#include "geometry/surfaceintersection.h"

#include <QString>
#include <optional>

struct ChamferLeadParameters
{
  double clearance = 5.0; // Axial clearance at the free endpoint, mm.
  double spanDegrees = 30.0; // Ellipse-parameter span, (0, 360] degrees.
  int intervals = 16; // Legacy saved/QML setting; validated for compatibility.
};

enum class ChamferPhase { LeadIn, Machining, LeadOut };

struct ChamferPathParameters
{
  EdgeGeometry::Ellipse edge;
  V3d cylinderOrigin = V3d::Zero();
  V3d cylinderAxis = V3d::UnitZ();
  bool flipAxis = false;
  double chamferSize = 0.5; // Setback along the end plane in the longitudinal section, mm.
  double chamferAngleDegrees = 45.0; // Diagonal angle from the hole axis, in (0, 90).
  ChamferLeadParameters leadIn;
  ChamferLeadParameters leadOut;
  double stagingDistance = 10.0; // From plane/axis intersection along outward Z, mm.
};

struct ChamferPathSample
{
  // Leads follow the current machining parameter for edge point and radial normal.
  V3d edgePoint = V3d::Zero();
  V3d radialNormal = V3d::Zero();
  // BASE-relative TCP with X/Y/Z columns and origin. On machining, origin is
  // the chamfer-section midpoint and rotation is local frame * Rx(180 degrees).
  M4d tcp = M4d::Identity();
};

struct ChamferArcDifferential
{
  double speed = 0.0; // norm(d TCP origin / d progress), mm per unit progress.
  double speedDerivative = 0.0; // d speed / d progress, mm per progress squared.
};

struct ChamferPathResult;

struct ChamferSamplingParameters
{
  double positionTolerance = 0.01; // mm, deviation from linear interpolation.
  double orientationToleranceDegrees = 0.1; // Deviation from quaternion interpolation.
};

// Internal chamfer with a specified longitudinal-section angle.
// Owns numerical geometry, not source samples.
class ChamferPath
{
public:
  static ChamferPathResult create(ChamferPathParameters parameters);
  const ChamferPathParameters& parameters() const { return m_parameters; }
  const V3d& outwardAxis() const { return m_outwardAxis; }
  V3d stagingPoint() const;
  // Copies the selected free lead endpoint orientation to P_s.
  // Both have origin P_s and Y=-outwardAxis. Machining is not a staging side.
  std::optional<M4d> stagingPose(ChamferPhase lead) const;
  // Ellipse parameter in radians, not arc length or cylinder azimuth.
  // Zero starts at the major-axis endpoint; increasing values follow the
  // chosen outward axis.
  std::optional<ChamferPathSample> evaluate(double angleRad) const;
  // Spatial leads combine ellipse traversal with axial clearance; progress is not length fraction.
  // TCP tilt follows chamferAngle*sin(pi*u/2) on entry and chamferAngle*cos(pi*u/2) on exit.
  // Machining progress remains the ellipse-parameter fraction, not time.
  std::optional<ChamferPathSample> evaluatePhase(ChamferPhase phase, double progress) const;
  // Analytical TCP-origin derivatives only; no quaternion or joint calculations.
  std::optional<ChamferArcDifferential> phaseArcDifferential(ChamferPhase phase,
                                                           double progress) const;

private:
  struct PositionDerivatives
  {
    V3d first = V3d::Zero(); // Per radian of machining parameter.
    V3d second = V3d::Zero(); // Per radian squared.
  };

  explicit ChamferPath(ChamferPathParameters parameters);
  std::optional<PositionDerivatives> machiningDerivatives(double angleRad) const;
  std::optional<ChamferPathSample> machiningSample(double angleRad, bool includeOrientation) const;
  QString initializeLeadFrames();

  ChamferPathParameters m_parameters;
  V3d m_outwardAxis = V3d::UnitZ();
  double m_direction = 1.0;
  double m_chamferSine = 0.0;
  double m_chamferCosine = 1.0;
  double m_chamferTangent = 0.0;
  ChamferPathSample m_leadIn;
  ChamferPathSample m_leadOut;
};

struct ChamferPathResult
{
  std::optional<ChamferPath> path;
  QString error;
};

