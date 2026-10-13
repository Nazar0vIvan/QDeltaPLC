#pragma once

#include <QJsonObject>

#include "chamfercentralmotion.h"
#include "chamfermotion.h"

struct ChamferCentralJointSample
{
  double time = 0.0; // Seconds from lead-in start, unchanged prescribed timing.
  ChamferCentralPose target;
  V6d joints = V6d::Zero(); // Degrees, continuous winding.
};

struct ChamferCentralValidationData
{
  ChamferCentralMotion motion;
  ChamferRobotSetup robot;
  QVector<ChamferCentralJointSample> samples; // Predetermined animation points, not RSI cycles.
};

struct ChamferCentralValidationResult
{
  std::optional<ChamferCentralValidationData> data;
  QString error;
  QJsonObject diagnostics; // Owned worker result, also returned on failure.
};

// Geometry, calibrated robot/limits and initial branch seed define the IK problem.
// The seed selects configuration; no seed-to-start movement is validated here.
// Counts are set from curvature/time before IK; no interpolation refinement.
// Sampled checks are not analytic guarantees. No retiming or device side effects.
ChamferCentralValidationResult validateChamferCentral(const ChamferPath& path,
                                                      const ChamferRobotSetup& robot,
                                                      const V6d& initialSeed);
