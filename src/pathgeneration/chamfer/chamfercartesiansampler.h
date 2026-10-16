#pragma once

#include <QJsonObject>

#include "trajectories/chamfermotion.h"

struct ChamferCartesianSample
{
  double time = 0.0; // Analytical evaluation seconds from the clear lead-in start.
  double progress = 0.0; // Analytic parameter local to pose.phase.
  ChamferPlaybackPose pose; // Analytic BASE TCP; joints are an IK validation witness.
  double machiningSeconds = 0.0; // Previous sample interval's overlap with Machining.
  bool terminal = false;
  double controllerTime = 0.0; // Cycle end j*period; may exceed time for the terminal sample.
};

struct ChamferCartesianSamplingData
{
  QVector<ChamferCartesianSample> samples;
  // Lead-in start, machining start, lead-out start (full turn), terminal.
  std::array<ChamferPlaybackPose, 4> phasePoses{};
};

struct ChamferCartesianSamplingResult
{
  std::optional<ChamferCartesianSamplingData> data;
  QString error;
  QJsonObject diagnostics; // Owned worker result, also returned on failure.
};

// Offline direct analytical targets on the supplied clock. Exact terminal once;
// separate phase metadata does not insert controller cycles. Never retimes.
ChamferCartesianSamplingResult sampleChamferCartesian(const ChamferMotion& motion, double cyclePeriod);
