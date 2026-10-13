#pragma once

#include "geometry/surfaceintersection.h"

#include <QString>
#include <QVector>
#include <optional>
#include <array>

struct ChamferLeadParameters
{
  int intervals = 16; // Minimum intervals; refinement may add samples.
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
  double leadRadius = 5.0; // Shared lead-in/out circle radius, mm.
  ChamferLeadParameters leadIn;
  ChamferLeadParameters leadOut;
  double stagingDistance = 10.0; // From plane/axis intersection along outward Z, mm.
};

struct ChamferPathSample
{
  // Leads retain the fixed machining junction edge point and radial normal.
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
struct ChamferSamplingResult;

struct ChamferSamplingParameters
{
  double positionTolerance = 0.01; // mm, deviation from linear interpolation.
  double orientationToleranceDegrees = 0.1; // Deviation from quaternion interpolation.
};

struct ChamferCurvePoint
{
  double progress = 0.0; // Phase-local [0, 1].
  ChamferPathSample sample;
};

struct ChamferSampledPath
{
  QVector<ChamferCurvePoint> points;
  // Inclusive phase boundaries; neighboring phases share a single point.
  // At shared boundaries progress belongs to the ending phase (1).
  std::array<qsizetype, 4> boundaries{};
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
  // LeadIn selects approach attitude; LeadOut selects return attitude.
  // Both have origin P_s and Y=-outwardAxis. Machining is not a staging side.
  std::optional<M4d> stagingPose(ChamferPhase lead) const;
  // Ellipse parameter in radians, not arc length or cylinder azimuth.
  // Zero starts at the major-axis endpoint; increasing values follow the
  // chosen outward axis.
  std::optional<ChamferPathSample> evaluate(double angleRad) const;
  // Leads are planar circles; progress is their length fraction in [0, 1].
  // Each lead retains the respective machining junction's rotation throughout.
  // Machining progress remains the ellipse-parameter fraction, not time.
  std::optional<ChamferPathSample> evaluatePhase(ChamferPhase phase, double progress) const;
  // Analytical TCP-origin derivatives only; no quaternion or joint calculations.
  std::optional<ChamferArcDifferential> phaseArcDifferential(ChamferPhase phase,
                                                           double progress) const;
  ChamferSamplingResult sample(const ChamferSamplingParameters& parameters = {}) const;

private:
  struct PositionDerivatives
  {
    V3d first = V3d::Zero(); // Per radian of machining parameter.
    V3d second = V3d::Zero(); // Per radian squared.
  };

  struct LeadArc
  {
    ChamferPathSample junction;
    V3d tangent = V3d::Zero(); // Unit machining direction at the junction.
    V3d planeY = V3d::Zero(); // Unit outward-axis projection normal to tangent.
    V3d center = V3d::Zero();
    V3d freeEndpoint = V3d::Zero();
    double sweep = 0.0; // Radians.
    double length = 0.0; // mm.
    M3d stagingRotation = M3d::Identity(); // Axial orientation at P_s, not on the lead.
  };

  explicit ChamferPath(ChamferPathParameters parameters);
  std::optional<PositionDerivatives> machiningDerivatives(double angleRad) const;
  std::optional<LeadArc> makeLeadArc(ChamferPhase phase) const;
  QString initializeLeadArcs();

  ChamferPathParameters m_parameters;
  V3d m_outwardAxis = V3d::UnitZ();
  double m_direction = 1.0;
  LeadArc m_leadIn;
  LeadArc m_leadOut;
};

struct ChamferPathResult
{
  std::optional<ChamferPath> path;
  QString error;
};

struct ChamferSamplingResult
{
  std::optional<ChamferSampledPath> path;
  QString error;
};
