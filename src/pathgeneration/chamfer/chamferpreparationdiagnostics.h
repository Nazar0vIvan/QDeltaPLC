#pragma once

#include "3d/robot/kinematics/kr10kinematics.h"
#include <QElapsedTimer>
#include <QJsonObject>
#include <utility>

// Worker-local accounting. IK time includes the solver's internal FK calls;
// explicit FK counts below exclude those internal calls. No global counters.
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

// A scope snapshot returns elapsed work without mutating the worker's result.
class ChamferProfileScope
{
public:
  explicit ChamferProfileScope(QJsonObject before) : m_before(std::move(before))
  { m_timer.start(); }

  QJsonObject finish(const QJsonObject& after) const
  {
    QJsonObject result;
    for (const auto& key : {"ikCalls", "explicitFkCalls", "ikMs", "explicitFkMs"})
      result.insert(key, after.value(key).toDouble() - m_before.value(key).toDouble());
    result.insert("computationMs", m_timer.nsecsElapsed() / 1e6);
    return result;
  }

private:
  QJsonObject m_before;
  QElapsedTimer m_timer;
};
