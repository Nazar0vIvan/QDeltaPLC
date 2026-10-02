#pragma once

#include "geometry/surfaceintersection.h"

#include <QString>
#include <QVector>
#include <optional>

struct ChamferLeadParameters
{
  double clearance = 5.0; // Additional axial displacement in mm.
  double spanDegrees = 30.0; // Span of the ellipse parameter.
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
  ChamferLeadParameters leadIn;
  ChamferLeadParameters leadOut;
  double stagingDistance = 10.0; // From plane/axis intersection along outward Z, mm.
};

struct ChamferPathSample
{
  V3d edgePoint = V3d::Zero();
  V3d radialNormal = V3d::Zero();
  // BASE-relative TCP: local machining frame * Rx(180 degrees).
  // Columns X/Y/Z, then the midpoint of the chamfer section.
  M4d tcp = M4d::Identity();
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
  // chosen outward axis. Negative values support the lead-in.
  std::optional<ChamferPathSample> evaluate(double angleRad) const;
  // Leads follow quarter circles in normalized angular/axial coordinates.
  // Progress is a geometric parameter in [0, 1], not elapsed time or arc length.
  std::optional<ChamferPathSample> evaluatePhase(ChamferPhase phase, double progress) const;
  ChamferSamplingResult sample(const ChamferSamplingParameters& parameters = {}) const;

private:
  explicit ChamferPath(ChamferPathParameters parameters);

  ChamferPathParameters m_parameters;
  V3d m_outwardAxis = V3d::UnitZ();
  double m_direction = 1.0;
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
