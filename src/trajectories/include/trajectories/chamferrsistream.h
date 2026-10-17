#pragma once

#include "trajectories/chamfermotion.h"

#include <QVector>
#include <optional>

// Relative RSI corrections for the central phases (lead-in, machining, lead-out).
// XYZ in mm and Z-Y-X Euler ABC in degrees, BASE-oriented; A/B/C add directly
// to the starting TCP angles on the controller.
struct ChamferRsiStream
{
  V6d initialCoordinates = V6d::Zero(); // Lead-in start, principal Euler branch.
  V6d finalCoordinates = V6d::Zero(); // initialCoordinates + sum of increments.
  V3d stagingPoint = V3d::Zero(); // P_s, mm.
  // One increment per controller cycle after Start = 1, then settle zeros.
  QVector<V6d> increments;
  int motionCycles = 0; // Increments that carry motion, excluding settle zeros.
  double duration = 0.0; // Central movement seconds, not computation time.
  V6d minimumOffset = V6d::Zero(); // Cumulative correction range per component.
  V6d maximumOffset = V6d::Zero();
};

struct ChamferRsiStreamResult
{
  std::optional<ChamferRsiStream> stream;
  QString error;
};

// Copied-input worker operation on the 4 ms controller clock; never retimes.
ChamferRsiStreamResult sampleChamferRsiStream(const ChamferMotion& motion);
