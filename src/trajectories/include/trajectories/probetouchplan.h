#pragma once

#include "geometry/mathtypes.h"

#include <QString>
#include <QVector>
#include <optional>

// Fixed ring scan of the calibration sphere with the measuring TCP.
constexpr int ProbeTouchCount = 31;
constexpr double ProbeApproachGap = 3.0; // mm from the start shell to the contact shell.
constexpr double ProbeApproachFeed = 1.0; // mm/s, constant during Approach and Retract.

// Sphere and strut of the calibration fixture, mm in BASE.
struct ProbeTouchPlanParameters
{
  V3d sphereCentre = V3d::Zero();
  double sphereRadius = 12.5;
  double ballRadius = 1.5; // Ruby ball of the measuring TCP.
  double strutRadius = 8.9; // Vertical strut on the axis through the sphere centre.
  double strutRecess = 2.0; // Strut top above the lowest sphere point.

  // Ball-centre distance from the sphere centre at contact.
  double contactDistance() const { return sphereRadius + ballRadius; }
  // Distance from the ball to the strut solid, which extends downward from its top face.
  double strutClearance(const V3d& ball) const;
};

// The transfer into a touch is the CIRC on the start shell from the preceding start.
struct ProbeTouch
{
  M4d start = M4d::Identity();
  M4d contact = M4d::Identity();
  V3d normal = V3d::Zero(); // Outward sphere normal at the contact.
  bool unwindAfter = false; // Turn the wrist over at start after the Retract.
};

struct ProbeTouchPlan
{
  ProbeTouchPlanParameters parameters;
  M4d entry = M4d::Identity(); // Above the pole, with the rotation of T1.
  QVector<ProbeTouch> touches;
  M4d exit = M4d::Identity(); // Outward along the last normal, with the rotation of the last touch.
};

struct ProbeTouchPlanResult
{
  std::optional<ProbeTouchPlan> plan;
  QString error;
};

ProbeTouchPlanResult createProbeTouchPlan(const ProbeTouchPlanParameters& parameters);
