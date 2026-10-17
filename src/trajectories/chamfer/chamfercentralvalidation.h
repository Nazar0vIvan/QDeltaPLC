#pragma once

#include "trajectories/chamfercentralmotion.h"
#include "trajectories/chamfermotion.h"
#include "kinematics/kr10kinematics.h"

struct ChamferCentralJointSample
{
  double time = 0.0; // Seconds from lead-in start, unchanged prescribed timing.
  ChamferCentralPose target;
  RoboCrap3D::Kr10Kinematics::IkSolution solution; // Degrees with continuous winding, Status and Turn.
  M4d actualTcp = M4d::Identity(); // FK-derived TCP, retained for nodal-point construction.
};

struct ChamferCentralValidationData
{
  QVector<ChamferCentralJointSample> samples; // Predetermined animation points, not RSI cycles.
};

struct ChamferCentralValidationResult
{
  std::optional<ChamferCentralValidationData> data;
  QString error;
};

struct ChamferCentralPreviewSpan
{
  double start = 0.0;
  double end = 0.0;
  int intervals = 0;
  QString phase;
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

private:
  QString prepare();

  const ChamferPath& m_path;
  const ChamferTimingParameters& m_timing;
  std::optional<ChamferCentralMotion> m_motion;
  QVector<ChamferCentralPreviewSpan> m_spans;
  qsizetype m_plannedSamples = 0;
  bool m_attempted = false;
  QString m_error;
  mutable QVector<std::optional<ChamferCentralPose>> m_targets;
};
