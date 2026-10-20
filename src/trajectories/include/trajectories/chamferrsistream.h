#pragma once

#include "trajectories/chamfermotion.h"

#include <QVector>
#include <optional>

// Relative RSI corrections for the central phases (lead-in, machining, lead-out).
// XYZ in mm and Z-Y-X Euler ABC in degrees, with POSCORR1 RefCorrSys = World.
// Compose each offset rotation before the preceding TCP rotation; add XYZ separately.
struct ChamferRsiStream
{
  V6d initialCoordinates = V6d::Zero(); // Six-decimal LeadStart XYZABC, principal Euler branch.
  V6d finalCoordinates = V6d::Zero(); // Six-decimal composed LeadEnd XYZABC, principal branch.
  V3d stagingPoint = V3d::Zero(); // P_s, mm.
  V6d toolCoordinates = V6d::Zero(); // Saved flange-to-TCP transform for KRL $TOOL.
  // One increment per controller cycle after Start = 1, then settle zeros.
  QVector<V6d> increments;
  QString diagnosticId; // Shared number in the plan and its packet-log filenames.
  int motionCycles = 0; // Increments that carry motion, excluding settle zeros.
  double duration = 0.0; // Central movement seconds, not computation time.
  double maximumTranslationOffset = 0.0; // Largest |X|, |Y| or |Z| displacement from LeadStart, mm.
  double maximumRotationOffset = 0.0; // Largest shortest rotation from LeadStart, [0, 180] deg.
};

struct ChamferRsiStreamResult
{
  std::optional<ChamferRsiStream> stream;
  QString error;
};

// Copied-input worker operation on the 4 ms controller clock; never retimes.
ChamferRsiStreamResult sampleChamferRsiStream(const ChamferMotion& motion);
