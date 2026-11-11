#pragma once

#include "geometry/mathtypes.h"

#include <QString>
#include <QVector>
#include <optional>

// Calibration and verification touches of the calibration sphere with the measuring TCP. The probe
// keeps one fixed orientation per pass; transfers are CIRC arcs on the start shell.
constexpr double ProbeApproachGap = 3.0; // mm from the start shell to the contact shell.

enum class ProbeTouchPurpose { Calibration, Verification };

// Sphere and strut of the calibration fixture, mm in BASE.
struct ProbeTouchPlanParameters
{
  V3d sphereCentre = V3d::Zero();
  double sphereRadius = 12.5;
  double ballRadius = 1.5; // Ruby ball of the measuring TCP.
  double strutRadius = 8.9; // Vertical strut on the axis through the sphere centre.
  double strutRecess = 2.0; // Strut top above the lowest sphere point.
  double searchFeed = 1.0; // mm/s, constant during every Approach; Retract uses the transfer limits.

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
  double tilt = 0.0; // deg, between the stylus and the approach direction.
  double roll = 0.0; // deg, about the stylus.
};

// One execution of a group's direction list in the group's fixed probe orientation.
struct ProbeTouchPass
{
  ProbeTouchPurpose purpose = ProbeTouchPurpose::Calibration;
  int group = 1; // G1 stylus down, G2 stylus along +X, G3 stylus down turned 180 deg about it.
  int repetition = 1;
  M4d entry = M4d::Identity(); // Outward and above the first touch.
  QVector<ProbeTouch> touches;
  M4d exit = M4d::Identity(); // Outward along the last normal.

  // "Calibration G2 pass 1".
  QString label() const;
};

struct ProbeTouchPlan
{
  ProbeTouchPlanParameters parameters;
  QVector<ProbeTouchPass> passes; // Calibration passes first, then verification passes.

  int touchCount(ProbeTouchPurpose purpose) const;
};

struct ProbeTouchPlanResult
{
  std::optional<ProbeTouchPlan> plan;
  QString error;
};

ProbeTouchPlanResult createProbeTouchPlan(const ProbeTouchPlanParameters& parameters);
