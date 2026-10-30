#pragma once

#include "trajectories/chamfercentralmotion.h"
#include "trajectories/timedmotion.h"

struct ChamferMotionResult;

// Timed simulation motion; does not emulate controller-specific KRL interpolation.
class ChamferMotion
{
public:
  static ChamferMotionResult create(const ChamferPath& path, const ChamferRobotSetup& robot);
  const QVector<ChamferJointPoint>& points() const { return m_timed.points(); }
  // Inclusive ranges, in ChamferMotionPhase order. Junction points are shared.
  const std::array<qsizetype, 8>& boundaries() const { return m_boundaries; }
  const ChamferRobotSetup& robot() const { return m_timed.robot(); }
  const ChamferPathParameters& parameters() const { return m_parameters; }
  double duration() const { return m_timed.duration(); }
  const std::optional<ChamferCentralMotion>& centralMotion() const { return m_centralMotion; }
  // The points with their playback segments (PTP smoothstep, LIN/central splines) and phase labels.
  const TimedMotion& timed() const { return m_timed; }

private:
  ChamferMotion(const ChamferPath& path, TimedMotion timed, const std::array<qsizetype, 8>& boundaries);
  void setCentralMotion(ChamferCentralMotion motion);
  ChamferPathParameters m_parameters;
  TimedMotion m_timed;
  std::array<qsizetype, 8> m_boundaries{};
  std::optional<ChamferCentralMotion> m_centralMotion;
};

struct ChamferMotionResult
{
  std::optional<ChamferMotion> motion;
  QString error;
};
