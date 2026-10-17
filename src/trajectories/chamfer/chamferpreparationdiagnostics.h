#pragma once

#include "kinematics/kr10kinematics.h"
#include <QElapsedTimer>
#include <QJsonObject>
#include <algorithm>
#include <utility>

// Worker-local accounting. Checked compatibility IK includes internal FK;
// angle-only IK includes its three-joint wrist prefix. Neither is explicit FK.
class ChamferProfiledKinematics
{
public:
  explicit ChamferProfiledKinematics(const RoboCrap3D::Kr10KinematicModel& model)
    : m_solver(model) {}

  auto solveIK(const M4d& target, const V6d& seed) const
  {
    QElapsedTimer timer;
    timer.start();
    auto result = m_solver.solveIK(target, seed);
    m_ikNanoseconds += timer.nsecsElapsed();
    ++m_ikCalls;
    return result;
  }

  auto solveFK(const V6d& joints) const
  {
    QElapsedTimer timer;
    timer.start();
    auto result = m_solver.solveFK(joints);
    m_fkNanoseconds += timer.nsecsElapsed();
    ++m_fkCalls;
    return result;
  }

  auto calculateIK(const M4d& target, const RoboCrap3D::Kr10Kinematics::IkSolution& previous) const
  {
    QElapsedTimer timer;
    timer.start();
    auto result = m_solver.calculateIK(target, previous);
    m_ikNanoseconds += timer.nsecsElapsed();
    ++m_ikCalls;
    return result;
  }

  auto configuration(const V6d& joints, const std::array<M4d, RoboCrap3D::LinkCount>& transforms) const
  {
    return m_solver.configuration(joints, transforms);
  }

  bool isValid() const { return m_solver.isValid(); }
  QJsonObject diagnostics() const
  {
    return {{"ikCalls", m_ikCalls}, {"explicitFkCalls", m_fkCalls},
            {"ikMs", m_ikNanoseconds / 1e6}, {"explicitFkMs", m_fkNanoseconds / 1e6}};
  }

private:
  RoboCrap3D::Kr10Kinematics m_solver;
  mutable qint64 m_ikCalls = 0;
  mutable qint64 m_fkCalls = 0;
  mutable qint64 m_ikNanoseconds = 0;
  mutable qint64 m_fkNanoseconds = 0;
};
