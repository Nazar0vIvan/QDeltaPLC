#include "chamferjob.h"

#include <cmath>
#include <utility>

namespace {

constexpr double kCyclePeriod = 0.004;

} // namespace

PreparedChamferJob::PreparedChamferJob(ChamferJobSource source)
  : m_source(std::move(source)) {}

double PreparedChamferJob::cyclePeriod() const
{
  return kCyclePeriod;
}

PreparedChamferJobResult PreparedChamferJob::create(ChamferJobSource source,
                                                    RsiCorrectionLimitConfiguration limits)
{
  if (!source.motion || source.jobId.isEmpty() || source.pathId == 0)
    return {{}, QStringLiteral("A saved trajectory and job identity are required.")};

  const auto& motion = *source.motion;
  const auto& points = motion.points();
  const auto& boundaries = motion.boundaries();
  if (points.isEmpty() || boundaries.front() != 0 || boundaries.back() != points.size() - 1)
    return {{}, QStringLiteral("The saved trajectory has invalid phase boundaries.")};
  if (!std::isfinite(motion.centralTimeScale()) || motion.centralTimeScale() < 1.0)
    return {{}, QStringLiteral("The saved trajectory has invalid effective timing.")};

  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    const auto index = boundaries[i];
    if (index < 0 || index >= points.size() || (i > 0 && index < boundaries[i - 1]))
      return {{}, QStringLiteral("The saved trajectory has invalid phase boundaries.")};
    const auto& point = points[index];
    if (!std::isfinite(point.time) || point.time < 0.0 || !point.tcp.allFinite()
        || !point.joints.allFinite()
        || (i > 0 && point.time < points[boundaries[i - 1]].time))
      return {{}, QStringLiteral("The saved trajectory has invalid boundary data.")};
  }

  auto job = std::shared_ptr<PreparedChamferJob>(new PreparedChamferJob(std::move(source)));
  job->m_motionStartTime = points[boundaries[2]].time;
  for (std::size_t i = 0; i < job->m_phaseTimes.size(); ++i) {
    const double time = points[boundaries[i + 2]].time - job->m_motionStartTime;
    if (!std::isfinite(time) || (i > 0 && time <= job->m_phaseTimes[i - 1]))
      return {{}, QStringLiteral("Lead-in, machining and lead-out must have positive durations.")};
    job->m_phaseTimes[i] = time;
  }

  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    const auto& point = points[boundaries[i]];
    const auto pose = motion.evaluate(point.time);
    if (!pose)
      return {{}, QStringLiteral("Cannot evaluate the saved trajectory boundary %1.")
                      .arg(static_cast<qulonglong>(i))};
    if ((pose->tcp.block<3, 1>(0, 3) - point.tcp.block<3, 1>(0, 3)).norm()
            > RoboCrap3D::Kr10PosEps
        || (pose->tcp.block<3, 3>(0, 0) - point.tcp.block<3, 3>(0, 0)).cwiseAbs().maxCoeff()
            > RoboCrap3D::Kr10RotEps)
      return {{}, QStringLiteral("The saved trajectory boundary does not match its evaluator.")};
    job->m_boundaryPoses[i] = *pose;
  }
  auto sampled = sampleChamferCartesian(motion, job->cyclePeriod());
  if (!sampled.data) return {{}, sampled.error};
  // Central boundary poses use the same analytic targets as the sample vector.
  // This retains the full-turn endpoint separately from phase-start metadata.
  for (std::size_t i = 0; i < sampled.data->phasePoses.size(); ++i)
    job->m_boundaryPoses[i + 2] = sampled.data->phasePoses[i];
  job->m_boundaryPoses[4].phase = ChamferMotionPhase::Machining;
  job->m_samples = std::move(sampled.data->samples);
  // Encoding/limit failures do not discard valid nominal samples. They have
  // their own readiness/error state on the same job.
  job->m_encoding = prepareRsiNominalEncoding(job->m_samples, limits);
  return {std::move(job), {}};
}
