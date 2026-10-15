#pragma once

#include <QJsonObject>

#include "chamfercentralmotion.h"
#include "chamfermotion.h"
#include "3d/robot/kinematics/kr10kinematics.h"

struct ChamferCentralJointSample
{
  double time = 0.0; // Seconds from lead-in start, unchanged prescribed timing.
  ChamferCentralPose target;
  RoboCrap3D::Kr10Kinematics::IkSolution solution; // Degrees with continuous winding, Status and Turn.
};

struct ChamferCentralValidationData
{
  QVector<ChamferCentralJointSample> samples; // Predetermined animation points, not RSI cycles.
};

struct ChamferCentralValidationResult
{
  std::optional<ChamferCentralValidationData> data;
  QString error;
  QJsonObject diagnostics; // Owned worker result, also returned on failure.
};

struct ChamferCentralPreviewSpan
{
  double start = 0.0;
  double end = 0.0;
  double accelerationBound = 0.0; // mm/s^2, geometric criterion, not a robot limit.
  int intervals = 0;
  QString phase;
  double curvatureBound = 0.0; // per mm
};

// Internal Generate stage. Borrowed path/timing outlive this worker-local cache.
// Construction is lazy, after the first candidate's incoming LIN and seed checks.
class ChamferCentralPreparation
{
public:
  ChamferCentralPreparation(const ChamferPath& path, const ChamferTimingParameters& timing);

  // Each call solves/checks a fresh joint sequence; only Cartesian work is shared.
  ChamferCentralValidationResult validate(const ChamferRobotSetup& robot,
                                        const RoboCrap3D::Kr10Kinematics::IkSolution& initialSeed);
  // Available after successful validate(); retained through later candidate failures.
  const ChamferCentralMotion& motion() const { return *m_motion; }
  const QVector<ChamferCentralPreviewSpan>& spans() const { return m_spans; }
  qsizetype plannedSamples() const { return m_plannedSamples; }
  // Logical-const memoization of scheduled targets, including a failed evaluation.
  // Calls visit a consecutive prefix; a cache hit returns the original result.
  std::optional<ChamferCentralPose> evaluateTarget(qsizetype index, double seconds) const;
  qint64 computationNanoseconds() const { return m_preparationNanoseconds + m_targetNanoseconds; }
  QJsonObject diagnostics() const;

private:
  QString prepare();

  const ChamferPath& m_path;
  const ChamferTimingParameters& m_timing;
  std::optional<ChamferCentralMotion> m_motion;
  QVector<ChamferCentralPreviewSpan> m_spans;
  qsizetype m_plannedSamples = 0;
  bool m_attempted = false;
  QString m_error;
  QJsonObject m_diagnostics;
  qint64 m_preparationNanoseconds = 0;
  mutable QVector<std::optional<ChamferCentralPose>> m_targets;
  mutable qint64 m_targetNanoseconds = 0;
  mutable qint64 m_targetCacheHits = 0;
};
