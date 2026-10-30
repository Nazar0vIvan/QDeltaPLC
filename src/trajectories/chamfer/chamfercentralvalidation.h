#pragma once

#include "trajectories/chamfercentralmotion.h"
#include "../motion/motionbuilder.h"

struct ChamferCentralPreviewSpan
{
  double start = 0.0;
  double end = 0.0;
  int intervals = 0;
  QString phase; // "Lead-in", "Machining" or "Lead-out".
};

// Internal Generate stage. Borrowed path/timing outlive this worker-local cache.
// Construction is lazy, after the first candidate's incoming LIN.
class ChamferCentralPreparation
{
public:
  ChamferCentralPreparation(const ChamferPath& path, const ChamferTimingParameters& timing);

  // Builds the central motion and its point schedule once; later calls return the first result.
  QString prepare();
  // Available after a successful prepare(); retained through later candidate failures.
  const ChamferCentralMotion& motion() const { return *m_motion; }
  const QVector<ChamferCentralPreviewSpan>& spans() const { return m_spans; }
  qsizetype plannedSamples() const { return m_plannedSamples; }
  // Logical-const memoization of scheduled targets, including a failed evaluation.
  // Calls visit a consecutive prefix; a cache hit returns the original result.
  std::optional<ChamferCentralPose> evaluateTarget(qsizetype index, double seconds) const;

private:
  const ChamferPath& m_path;
  const ChamferTimingParameters& m_timing;
  std::optional<ChamferCentralMotion> m_motion;
  QVector<ChamferCentralPreviewSpan> m_spans;
  qsizetype m_plannedSamples = 0;
  bool m_attempted = false;
  QString m_error;
  mutable QVector<std::optional<ChamferCentralPose>> m_targets;
};

// The lead-in, machining and lead-out of a prepared central motion as one Cartesian path.
class CentralPath final : public TimedPath
{
public:
  explicit CentralPath(const ChamferCentralPreparation& preparation) : m_preparation(preparation) {}
  PathSchedule schedule() const override;
  std::optional<PathTarget> target(qsizetype index, double seconds) const override;

private:
  const ChamferCentralPreparation& m_preparation;
};
