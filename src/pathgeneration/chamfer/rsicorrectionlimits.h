#pragma once

#include "geometry/mathtypes.h"

#include <QByteArray>
#include <QString>
#include <optional>

// Application bounds, independent of the controller object's monitored quantity.
// X/Y/Z are mm; unwrapped A/B/C are degrees. Zero must be allowed for holds.
struct RsiCorrectionBounds
{
  V6d minimum = V6d::Zero();
  V6d maximum = V6d::Zero();
};

struct RsiPosCorrLimits
{
  V3d minimumTranslation = V3d::Zero();
  V3d maximumTranslation = V3d::Zero();
  double maximumRotationDegrees = 0.0; // Absolute cumulative change in each A/B/C component.
};

struct RsiPosCorrMonLimits
{
  double maximumTranslationMillimetres = 0.0; // Absolute cumulative change in each X/Y/Z component.
  double maximumRotationDegrees = 0.0; // Absolute cumulative change in each A/B/C component.
};

struct RsiCorrectionLimitProfile
{
  QString id;
  RsiCorrectionBounds increment;
  RsiCorrectionBounds cumulative;
  RsiPosCorrLimits poscorr;
  RsiPosCorrMonLimits poscorrmon;
};

struct RsiCorrectionLimitConfiguration
{
  std::optional<RsiCorrectionLimitProfile> profile;
  QString error;
  QByteArray revision; // Content SHA-256; unavailable inputs have explicit sentinel revisions.
  bool supplied = false;
};

enum class RsiCorrectionLimitStatus {
  Unconfigured, InvalidConfiguration, Exceeded, Passed
};

struct RsiCorrectionLimitViolation
{
  qsizetype sampleIndex = 0;
  QString quantity;
  QString unit;
  double value = 0.0;
  double minimum = 0.0;
  double maximum = 0.0;
};

struct RsiCorrectionLimitValidation
{
  RsiCorrectionLimitStatus status = RsiCorrectionLimitStatus::Unconfigured;
  std::optional<RsiCorrectionLimitViolation> violation;
  QString reason;
  bool ready() const { return status == RsiCorrectionLimitStatus::Passed; }
};

// Syntax/finite-bound checks do not verify installation or firmware parameter ranges.
QString validateRsiCorrectionLimitProfile(const RsiCorrectionLimitProfile& profile);
RsiCorrectionLimitConfiguration readRsiCorrectionLimitConfiguration(const QString& path);
QString describeRsiCorrectionLimitViolation(const RsiCorrectionLimitViolation& violation);
