#include "trajectories/probetouchplan.h"

#include "geometry/utils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {

constexpr double EntryDistance = 60.0; // mm from the sphere centre along the first normal...
constexpr double EntryRise = 80.0; // ...and this much higher.
constexpr double ExitDistance = 42.0; // mm from the sphere centre along the last normal.

// Fixed probe orientation of a group, KUKA A, B, C in deg: G1 stylus down, G2 stylus along +X (as at
// HOME), G3 stylus down with the flange turned 180 deg about it. G1 and G2 are the workpiece
// orientations; G3 adds the turn about another axis without which the ball centre is not determined.
struct GroupOrientation
{
  double a;
  double b;
  double c;
};

constexpr std::array<GroupOrientation, 3> Groups{{{180.0, 0.0, 180.0}, {0.0, 90.0, 0.0}, {0.0, 0.0, 180.0}}};

// Approach direction in the probe frame: tilt from the stylus and roll about it, deg.
struct Direction
{
  double tilt;
  double roll;
};

// The same directions in every calibration group, each reachable in G2 (sin tilt * cos roll <= 0.5
// keeps the touch point above latitude -30 deg, clear of the strut): tilt 0, tilt 30 at six equally
// spaced rolls, tilts 45-90 at rolls 60, 180, 300. Listed in the order of the shortest total CIRC
// transfer time from tilt 0 whose arcs keep the strut clearance in G2 (exact search, scratch); arc
// lengths do not depend on the group, so the order is shortest in every group.
constexpr std::array<Direction, 19> CalibrationDirections{{
    {0, 0}, {30, 240},
    {90, 300}, {75, 300}, {60, 300}, {45, 300}, {30, 300},
    {30, 0}, {30, 60}, {45, 60}, {60, 60}, {75, 60}, {90, 60},
    {30, 120}, {30, 180}, {45, 180}, {60, 180}, {75, 180}, {90, 180}}};

// Rolls offset by 30 deg from the calibration rings; touches unreachable in a group are skipped.
// Tilt 0, the tilt-15 ring, then meridians up and down; without the rolls 30 and 330, which G2
// skips at tilt 45 and above, the order stays a chain of neighbours.
constexpr std::array<Direction, 25> VerificationDirections{{
    {0, 0},
    {15, 30}, {15, 330}, {15, 270}, {15, 210}, {15, 150}, {15, 90},
    {45, 90}, {75, 90}, {90, 90}, {90, 150}, {75, 150}, {45, 150},
    {45, 210}, {75, 210}, {90, 210}, {90, 270}, {75, 270}, {45, 270},
    {45, 330}, {75, 330}, {90, 330}, {90, 30}, {75, 30}, {45, 30}}};

struct PassRow
{
  ProbeTouchPurpose purpose;
  int group;
  int repetition;
  bool reversed;
};

// Mirrored calibration order: under a drift linear in time every group and direction has the same
// mean time. Verification follows in the two workpiece orientations.
constexpr std::array<PassRow, 8> Passes{{{ProbeTouchPurpose::Calibration, 1, 1, false},
                                         {ProbeTouchPurpose::Calibration, 2, 1, false},
                                         {ProbeTouchPurpose::Calibration, 3, 1, false},
                                         {ProbeTouchPurpose::Calibration, 3, 2, true},
                                         {ProbeTouchPurpose::Calibration, 2, 2, true},
                                         {ProbeTouchPurpose::Calibration, 1, 2, true},
                                         {ProbeTouchPurpose::Verification, 1, 1, false},
                                         {ProbeTouchPurpose::Verification, 2, 1, false}}};

bool positive(double value)
{
  return std::isfinite(value) && value > 0.0;
}

QVector<Direction> passDirections(const PassRow& row)
{
  QVector<Direction> directions = row.purpose == ProbeTouchPurpose::Calibration
      ? QVector<Direction>(CalibrationDirections.begin(), CalibrationDirections.end())
      : QVector<Direction>(VerificationDirections.begin(), VerificationDirections.end());
  if (row.reversed) std::reverse(directions.begin(), directions.end());
  return directions;
}

// The approach direction e = (-sin tilt cos roll, sin tilt sin roll, cos tilt) in the probe frame
// gives the outward normal n = -R e; the contact lies on n at the contact distance.
ProbeTouch makeTouch(const ProbeTouchPlanParameters& parameters, const M3d& rotation, const Direction& direction)
{
  const double tilt = direction.tilt * GeomConst::DegToRad;
  const double roll = direction.roll * GeomConst::DegToRad;
  const V3d approach(-std::sin(tilt) * std::cos(roll), std::sin(tilt) * std::sin(roll), std::cos(tilt));
  const V3d normal = -(rotation * approach);
  const V3d& centre = parameters.sphereCentre;
  const double contact = parameters.contactDistance();
  return {makeTransform(rotation, centre + (contact + ProbeApproachGap) * normal),
          makeTransform(rotation, centre + contact * normal), normal, direction.tilt, direction.roll};
}

} // namespace

double ProbeTouchPlanParameters::strutClearance(const V3d& ball) const
{
  const V3d offset = ball - sphereCentre;
  const double outside = std::hypot(offset.x(), offset.y()) - strutRadius;
  const double above = offset.z() + sphereRadius - strutRecess;
  const double distance = outside <= 0.0 ? above : (above <= 0.0 ? outside : std::hypot(outside, above));
  return distance - ballRadius;
}

QString ProbeTouchPass::label() const
{
  const QString name = purpose == ProbeTouchPurpose::Calibration ? QStringLiteral("Calibration")
                                                                 : QStringLiteral("Verification");
  return QStringLiteral("%1 G%2 pass %3").arg(name).arg(group).arg(repetition);
}

int ProbeTouchPlan::touchCount(ProbeTouchPurpose purpose) const
{
  qsizetype count = 0;
  for (const ProbeTouchPass& pass : passes)
    if (pass.purpose == purpose) count += pass.touches.size();
  return static_cast<int>(count);
}

ProbeTouchPlanResult createProbeTouchPlan(const ProbeTouchPlanParameters& parameters)
{
  if (!parameters.sphereCentre.allFinite() || !positive(parameters.sphereRadius)
      || !positive(parameters.ballRadius) || !positive(parameters.strutRadius)
      || !positive(parameters.strutRecess) || parameters.strutRecess >= 2.0 * parameters.sphereRadius
      || !positive(parameters.searchFeed))
    return {{}, QStringLiteral("Invalid sphere centre, radii, strut recess or search feed.")};
  const V3d& centre = parameters.sphereCentre;
  ProbeTouchPlan plan{parameters, {}};
  for (const PassRow& row : Passes) {
    const GroupOrientation& group = Groups[static_cast<std::size_t>(row.group - 1)];
    const M3d rotation = euler2rot(group.a, group.b, group.c);
    ProbeTouchPass pass{row.purpose, row.group, row.repetition, M4d::Identity(), {}, M4d::Identity()};
    const QVector<Direction> directions = passDirections(row);
    for (qsizetype index = 0; index < directions.size(); ++index) {
      const ProbeTouch touch = makeTouch(parameters, rotation, directions[index]);
      const double clearance = std::min(parameters.strutClearance(touch.start.block<3, 1>(0, 3)),
                                        parameters.strutClearance(touch.contact.block<3, 1>(0, 3)));
      if (clearance >= ProbeApproachGap) pass.touches.append(touch);
      // Calibration directions must be the same in every group; verification keeps the reachable ones.
      else if (row.purpose == ProbeTouchPurpose::Calibration)
        return {{}, QStringLiteral("%1, touch %2/%3: strut clearance %4 mm is below %5 mm.")
                        .arg(pass.label()).arg(index + 1).arg(directions.size())
                        .arg(clearance, 0, 'f', 4).arg(ProbeApproachGap)};
    }
    if (pass.touches.isEmpty()) return {{}, pass.label() + QStringLiteral(": no reachable touch.")};
    pass.entry = makeTransform(rotation, centre + EntryDistance * pass.touches.front().normal + EntryRise * V3d::UnitZ());
    pass.exit = makeTransform(rotation, centre + ExitDistance * pass.touches.back().normal);
    plan.passes.append(std::move(pass));
  }
  return {std::move(plan), {}};
}
