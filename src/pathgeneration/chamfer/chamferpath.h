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
  double chamferSize = 0.0; // Leg dimension in mm, e.g. 2 in 2 x 45 degrees.
  ChamferLeadParameters leadIn;
  ChamferLeadParameters leadOut;
};

struct ChamferPathSample
{
  V3d edgePoint = V3d::Zero();
  V3d radialNormal = V3d::Zero();
  // BASE-relative surface-attached TCP: columns X/Y/Z, then origin.
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

// Nominal internal 45-degree chamfer. Owns numerical geometry, not source samples.
class ChamferPath
{
public:
  static ChamferPathResult create(ChamferPathParameters parameters);
  const ChamferPathParameters& parameters() const { return m_parameters; }
  const V3d& outwardAxis() const { return m_outwardAxis; }
  // Ellipse parameter in radians, not arc length or cylinder azimuth.
  // Zero starts at the major-axis endpoint; increasing values follow the
  // chosen outward axis. Negative values support the lead-in.
  std::optional<ChamferPathSample> evaluate(double angleRad) const;
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
