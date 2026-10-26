#pragma once

#include "trajectories/chamferpath.h"

#include <array>
#include <QVector>

struct ChamferArcLengthParameters
{
  // u is phase progress, s is distance in mm, g = norm(d TCP origin / du).
  // Numerical error estimates, independent of display sampling or robot limits.
  double lengthTolerance = 1e-9; // Total quadrature error budget, mm.
  double lookupTolerance = 1e-10; // Local inverse distance residual, mm.
  double relativeSpeedTolerance = 1e-8; // abs(g * du/ds - 1).
  double speedDerivativeTolerance = 1e-7; // abs(d(g * du/ds)/ds), per mm.
};

struct ChamferArcLengthKnot
{
  double progress = 0.0;
  double distance = 0.0; // Accumulated TCP-origin distance from this phase's start, mm.
};

struct ChamferArcLengthResult;

// Immutable numerical distance map for one spatial lead or machining phase. No timing or IK.
// Knots and quintics approximate inverse arc length, not poses or RSI clock samples.
class ChamferArcLength
{
public:
  static ChamferArcLengthResult create(const ChamferPath& path, ChamferPhase phase,
                                      const ChamferArcLengthParameters& parameters = {});

  double length() const { return m_knots.last().distance; }
  // Locate the distance interval by binary search and evaluate its saved quintic.
  // Evaluate the returned progress with the original ChamferPath::evaluatePhase;
  // do not interpolate XYZ or orientation between table entries.
  std::optional<double> progressAtDistance(double distance) const;

private:
  // One normalized inverse polynomial per consecutive phase knot pair.
  ChamferArcLength(QVector<ChamferArcLengthKnot> knots,
                   QVector<std::array<double, 6>> polynomials);

  QVector<ChamferArcLengthKnot> m_knots;
  QVector<std::array<double, 6>> m_polynomials;
};

struct ChamferArcLengthResult
{
  std::optional<ChamferArcLength> arcLength;
  QString error;
};
