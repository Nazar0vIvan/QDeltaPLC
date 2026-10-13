#include "chamfercentralmotion.h"

#include "chamfermotion.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

std::optional<std::size_t> phaseIndex(ChamferPhase phase)
{
  switch (phase) {
  case ChamferPhase::LeadIn: return 0;
  case ChamferPhase::Machining: return 1;
  case ChamferPhase::LeadOut: return 2;
  }
  return std::nullopt;
}

bool positiveFinite(double value)
{
  return std::isfinite(value) && value > 0.0;
}

bool validProfile(const ChamferCentralProfile& profile)
{
  return positiveFinite(profile.length) && positiveFinite(profile.speed)
      && positiveFinite(profile.duration())
      && std::isfinite(profile.cruiseSeconds) && profile.cruiseSeconds >= 0.0;
}

struct DistanceState
{
  double distance = 0.0;
  double speed = 0.0;
  double acceleration = 0.0;
};

DistanceState leadInState(const ChamferCentralProfile& profile, double seconds)
{
  if (seconds == profile.duration()) return {profile.length, profile.speed, 0.0};
  if (seconds < profile.rampSeconds) {
    const double fraction = seconds / profile.rampSeconds;
    return {profile.rampDistance * fraction * fraction,
            profile.speed * fraction, profile.acceleration};
  }
  return {profile.rampDistance + profile.speed * (seconds - profile.rampSeconds),
          profile.speed, 0.0};
}

DistanceState leadOutState(const ChamferCentralProfile& profile, double seconds)
{
  if (seconds == profile.duration()) return {profile.length, 0.0, 0.0};
  if (seconds < profile.cruiseSeconds)
    return {profile.speed * seconds, profile.speed, 0.0};
  if (seconds == profile.cruiseSeconds)
    return {profile.length - profile.rampDistance, profile.speed, -profile.acceleration};
  // Remaining distance avoids subtracting two large travelled-distance terms
  // when evaluating near the final stop. No integration in time is accumulated.
  const double remaining = std::clamp((profile.duration() - seconds) / profile.rampSeconds,
                                    0.0, 1.0);
  return {profile.length - profile.rampDistance * remaining * remaining,
          profile.speed * remaining, -profile.acceleration};
}

} // namespace

ChamferCentralMotion::ChamferCentralMotion(const ChamferPath& path,
                                         std::array<ChamferArcLength, 3> lengths,
                                         const ChamferTimingParameters& timing)
  : m_path(path), m_lengths(std::move(lengths))
{
  for (std::size_t index = 0; index < m_profiles.size(); ++index) {
    m_profiles[index].length = m_lengths[index].length();
    m_profiles[index].speed = timing.machiningFeed;
  }
}

QString ChamferCentralMotion::initializeTiming(double fraction)
{
  for (std::size_t index = 0; index < m_profiles.size(); ++index) {
    auto& profile = m_profiles[index];
    if (index == 1) {
      profile.cruiseSeconds = profile.length / profile.speed;
    } else {
      profile.rampDistance = fraction * profile.length;
      // a = V^2/(2*f*L), evaluated without squaring V.
      profile.rampSeconds = 2.0 * (profile.rampDistance / profile.speed);
      profile.acceleration = profile.speed / profile.rampSeconds;
      profile.cruiseSeconds = (profile.length - profile.rampDistance) / profile.speed;
      if (!positiveFinite(profile.rampDistance) || !positiveFinite(profile.rampSeconds)
          || !positiveFinite(profile.acceleration)
          || (fraction < 1.0 && !positiveFinite(profile.cruiseSeconds)))
        return QStringLiteral("Lead ramp exceeds the numerical timing range.");
    }
    if (!validProfile(profile))
      return QStringLiteral("Central phase duration exceeds the numerical timing range.");
    const double start = m_phaseTimes[index];
    const double end = start + profile.duration();
    if (!positiveFinite(end) || end <= start)
      return QStringLiteral("Central phase boundaries cannot be represented distinctly.");
    if (index != 1 && profile.cruiseSeconds > 0.0) {
      const double change = start + (index == 0 ? profile.rampSeconds : profile.cruiseSeconds);
      if (change <= start || change >= end)
        return QStringLiteral("Lead ramp and cruise boundaries cannot be represented distinctly.");
    }
    m_phaseTimes[index + 1] = end;
  }
  return {};
}

ChamferCentralMotionResult ChamferCentralMotion::create(const ChamferPath& path,
                                                       const ChamferTimingParameters& timing)
{
  if (!positiveFinite(timing.machiningFeed))
    return {{}, QStringLiteral("Machining speed must be finite and positive.")};
  if (!positiveFinite(timing.accelerationDistanceFraction)
      || timing.accelerationDistanceFraction > 1.0)
    return {{}, QStringLiteral("Acceleration/deceleration distance must be in (0, 100] percent.")};
  auto leadIn = ChamferArcLength::create(path, ChamferPhase::LeadIn);
  if (!leadIn.arcLength) return {{}, leadIn.error};
  auto machining = ChamferArcLength::create(path, ChamferPhase::Machining);
  if (!machining.arcLength) return {{}, machining.error};
  auto leadOut = ChamferArcLength::create(path, ChamferPhase::LeadOut);
  if (!leadOut.arcLength) return {{}, leadOut.error};
  ChamferCentralMotion motion(path, {std::move(*leadIn.arcLength),
      std::move(*machining.arcLength), std::move(*leadOut.arcLength)}, timing);
  const QString error = motion.initializeTiming(timing.accelerationDistanceFraction);
  if (!error.isEmpty()) return {{}, error};
  return {std::move(motion), {}};
}

std::optional<ChamferCentralProfile> ChamferCentralMotion::profile(ChamferPhase phase) const
{
  const auto index = phaseIndex(phase);
  if (!index) return std::nullopt;
  return m_profiles[*index];
}

std::optional<ChamferCentralPose> ChamferCentralMotion::evaluate(double seconds) const
{
  if (!std::isfinite(seconds) || seconds < 0.0 || seconds > duration())
    return std::nullopt;
  if (seconds == duration())
    return evaluatePhase(ChamferPhase::LeadOut, m_profiles[2].duration());
  std::size_t index = 0;
  while (index < 2 && seconds >= m_phaseTimes[index + 1]) ++index;
  // Clamp only subtraction roundoff; the caller's time was checked above.
  const double local = std::clamp(seconds - m_phaseTimes[index], 0.0,
                                  m_profiles[index].duration());
  return evaluatePhase(static_cast<ChamferPhase>(index), local);
}

std::optional<ChamferCentralPose> ChamferCentralMotion::evaluatePhase(ChamferPhase phase,
                                                                  double seconds) const
{
  const auto index = phaseIndex(phase);
  if (!index || !std::isfinite(seconds)) return std::nullopt;
  const auto& profile = m_profiles[*index];
  if (seconds < 0.0 || seconds > profile.duration()) return std::nullopt;
  DistanceState state;
  switch (phase) {
  case ChamferPhase::LeadIn: state = leadInState(profile, seconds); break;
  case ChamferPhase::Machining: state = {profile.speed * seconds, profile.speed, 0.0}; break;
  case ChamferPhase::LeadOut: state = leadOutState(profile, seconds); break;
  }
  // Preserve shared analytic endpoints exactly; internal clamping handles only
  // floating-point arithmetic in the closed-form distance equations.
  if (seconds == 0.0) state.distance = 0.0;
  else if (seconds == profile.duration()) state.distance = profile.length;
  if (!std::isfinite(state.distance) || !std::isfinite(state.speed)
      || !std::isfinite(state.acceleration)) return std::nullopt;
  state.distance = std::clamp(state.distance, 0.0, profile.length);
  const auto progress = m_lengths[*index].progressAtDistance(state.distance);
  if (!progress) return std::nullopt;
  const auto sample = m_path.evaluatePhase(phase, *progress);
  if (!sample) return std::nullopt;
  ChamferCentralPose result;
  result.tcp = sample->tcp;
  result.phase = phase;
  result.progress = *progress;
  result.distance = state.distance;
  result.speed = state.speed;
  result.tangentialAcceleration = state.acceleration;
  return result;
}
