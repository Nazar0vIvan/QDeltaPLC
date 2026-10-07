#pragma once

#include "chamfercartesiansampler.h"
#include "rsicorrectionlimits.h"

// Sampled nominal geometric requirements in BASE, including zero while waiting.
// These values are not configured POSCORR/POSCORRMON limits. ABC values are
// unwrapped TCP Euler components (degrees), not principal rotation magnitudes.
struct RsiCorrectionEnvelope
{
  V3d minimumTranslationIncrement = V3d::Zero();
  V3d maximumTranslationIncrement = V3d::Zero();
  V3d minimumTranslationOffset = V3d::Zero();
  V3d maximumTranslationOffset = V3d::Zero();
  V3d minimumAngleIncrement = V3d::Zero();
  V3d maximumAngleIncrement = V3d::Zero();
  V3d minimumAngleOffset = V3d::Zero();
  V3d maximumAngleOffset = V3d::Zero();
  double maximumTranslationIncrementNorm = 0.0;
  double maximumTranslationOffsetNorm = 0.0;
  double maximumCycleRotationDegrees = 0.0;
  double maximumOffsetRotationDegrees = 0.0; // Principal angle, at most 180 degrees.
  double rotationTravelDegrees = 0.0; // Sum of sampled changes; retains full turns.
  double maximumTranslationReconstructionError = 0.0;
  double maximumRotationReconstructionError = 0.0; // Maximum rotation-matrix entry error.
};

// Future protocol integration must write these exact attribute strings unchanged.
struct RsiPoseCorrection
{
  V6d increment = V6d::Zero(); // Values parsed from the serialized attributes.
  std::array<QString, 6> attributes{}; // X/Y/Z/A/B/C, C locale, max_digits10.
};

struct RsiNominalEncoding
{
  // Absolute BASE TCP coordinates at sample zero. A/B/C add directly to these
  // starting angles; session validation must match the controller's Euler branch.
  V6d initialCoordinates = V6d::Zero();
  // Entry k corresponds to nominal sample k+1. Sample zero emits no increment.
  QVector<RsiPoseCorrection> corrections;
  RsiCorrectionEnvelope envelope;
  RsiCorrectionLimitConfiguration limitConfiguration;
  RsiCorrectionLimitValidation limitValidation;
  bool orientationRequired = false;
  QString unavailableReason;
  bool ready() const
  { return !corrections.isEmpty() && limitValidation.ready() && unavailableReason.isEmpty(); }
};

struct RsiNominalEncodingResult
{
  std::optional<RsiNominalEncoding> data;
  QString error; // Numerical failure; configuration/limit failures remain in data.
};

// Offline encoding/report preparation; contains no device or viewport dependencies.
RsiNominalEncodingResult prepareRsiNominalEncoding(const QVector<ChamferCartesianSample>& samples,
                                                  const RsiCorrectionLimitConfiguration& limits = {});
