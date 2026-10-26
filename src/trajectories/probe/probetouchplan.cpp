#include "trajectories/probetouchplan.h"

#include "geometry/utils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {

constexpr double EntryHeight = 60.0; // mm above the sphere centre.
constexpr double ExitDistance = 42.0; // mm from the sphere centre along the last normal.
constexpr double RollDegrees = 30.0;
constexpr std::size_t UnwindAfterTouch = 21; // A4 would pass +185 deg on the way to T22.

struct TouchRow
{
  double latitude; // deg
  double longitude; // deg; the pole uses the longitude of the following touch.
  double tilt; // deg, stylus turned in the meridian plane from -normal towards straight down.
};

constexpr std::array<TouchRow, ProbeTouchCount> Touches{{
    {90, 90, 0},
    {50, 90, 0}, {50, 45, 0}, {50, 0, 0}, {50, 315, 0}, {50, 270, 0}, {50, 225, 0},
    {20, 210, 0}, {20, 240, 0}, {20, 270, 0}, {20, 300, 30}, {20, 330, 30}, {20, 0, 30}, {20, 30, 30},
    {20, 60, 0}, {20, 90, 0}, {20, 120, 0},
    {-10, 60, 0}, {-10, 90, 0}, {-10, 120, 0}, {-10, 150, 0},
    {-10, 180, 0}, {-10, 210, 0}, {-10, 240, 0}, {-10, 270, 0},
    {-30, 240, 0}, {-30, 210, 0}, {-30, 180, 0}, {-30, 150, 0}, {-30, 120, 0}, {-30, 90, 0}}};

V3d sphereNormal(const TouchRow& row)
{
  const double latitude = row.latitude * GeomConst::DegToRad;
  const double longitude = row.longitude * GeomConst::DegToRad;
  return {std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude), std::sin(latitude)};
}

// Rz(longitude) Ry(90 - latitude - tilt) Rx(180) Rz(roll): the stylus (z) points down at tilt 0.
M3d touchRotation(const TouchRow& row)
{
  const M4d rotation = makeRotation(row.longitude, Axis::Z) * makeRotation(90.0 - row.latitude - row.tilt, Axis::Y)
      * makeRotation(180.0, Axis::X) * makeRotation(RollDegrees, Axis::Z);
  return rotation.block<3, 3>(0, 0);
}

// Waypoints on the start shell at most ProbeMaximumChordDegrees apart, rotations slerped
// between the two touches.
QVector<M4d> transferPoses(const ProbeTouch& from, const ProbeTouch& to, const V3d& centre)
{
  const double degrees = std::acos(std::clamp(from.normal.dot(to.normal), -1.0, 1.0)) * GeomConst::RadToDeg;
  const int chords = std::max(1, static_cast<int>(std::ceil(degrees / ProbeMaximumChordDegrees - 1e-9)));
  const double shell = (from.start.block<3, 1>(0, 3) - centre).norm();
  const Eigen::Quaterniond first(M3d(from.start.block<3, 3>(0, 0)));
  const Eigen::Quaterniond last(M3d(to.start.block<3, 3>(0, 0)));
  QVector<M4d> poses;
  for (int chord = 1; chord < chords; ++chord) {
    const double u = static_cast<double>(chord) / chords;
    const V3d direction = ((1.0 - u) * from.normal + u * to.normal).normalized();
    poses.append(makeTransform(first.slerp(u, last).toRotationMatrix(), centre + shell * direction));
  }
  poses.append(to.start);
  return poses;
}

bool positive(double value)
{
  return std::isfinite(value) && value > 0.0;
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

ProbeTouchPlanResult createProbeTouchPlan(const ProbeTouchPlanParameters& parameters)
{
  if (!parameters.sphereCentre.allFinite() || !positive(parameters.sphereRadius)
      || !positive(parameters.ballRadius) || !positive(parameters.strutRadius)
      || !positive(parameters.strutRecess) || parameters.strutRecess >= 2.0 * parameters.sphereRadius)
    return {{}, QStringLiteral("Invalid sphere centre, radii or strut recess.")};
  const V3d& centre = parameters.sphereCentre;
  const double contact = parameters.contactDistance();
  ProbeTouchPlan plan{parameters, M4d::Identity(), {}, M4d::Identity()};
  plan.touches.reserve(ProbeTouchCount);
  for (std::size_t index = 0; index < Touches.size(); ++index) {
    ProbeTouch touch;
    touch.normal = sphereNormal(Touches[index]);
    const M3d rotation = touchRotation(Touches[index]);
    touch.start = makeTransform(rotation, centre + (contact + ProbeApproachGap) * touch.normal);
    touch.contact = makeTransform(rotation, centre + contact * touch.normal);
    touch.unwindAfter = index + 1 == UnwindAfterTouch;
    const double clearance = std::min(parameters.strutClearance(touch.start.block<3, 1>(0, 3)),
                                      parameters.strutClearance(touch.contact.block<3, 1>(0, 3)));
    if (clearance < ProbeApproachGap)
      return {{}, QStringLiteral("Touch %1/%2: strut clearance %3 mm is below %4 mm.")
                      .arg(index + 1).arg(ProbeTouchCount).arg(clearance, 0, 'f', 4).arg(ProbeApproachGap)};
    if (!plan.touches.isEmpty()) touch.transfer = transferPoses(plan.touches.back(), touch, centre);
    plan.touches.append(touch);
  }
  const ProbeTouch& last = plan.touches.back();
  plan.entry = makeTransform(plan.touches.front().start.block<3, 3>(0, 0), centre + EntryHeight * V3d::UnitZ());
  plan.exit = makeTransform(last.start.block<3, 3>(0, 0), centre + ExitDistance * last.normal);
  return {std::move(plan), {}};
}
