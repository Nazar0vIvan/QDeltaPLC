#pragma once

#include "chamfermotion.h"

struct ChamferCartesianSample
{
  double time = 0.0; // Seconds from the clear lead-in start.
  double progress = 0.0; // Analytic parameter local to pose.phase.
  ChamferPlaybackPose pose; // Analytic BASE TCP; joints are an IK validation witness.
  double machiningSeconds = 0.0; // Previous sample interval's overlap with Machining.
  bool terminal = false;
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
};

// Offline Cartesian targets, preserving the saved motion's effective timeline.
ChamferCartesianSamplingResult sampleChamferCartesian(const ChamferMotion& motion, double cyclePeriod);
