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

struct TouchRow
{
  double latitude; // deg
  double longitude; // deg
  double tilt; // deg, stylus turned in the meridian plane from -normal towards straight down.
  double roll; // deg about the stylus; free for the axisymmetric probe, chosen to keep A6 at 90-150 deg.
};

// A uniform grid about 30 deg apart: the pole, latitude 60 every 60 deg, 30 every 36 deg, 0 every
// 30 deg and -30 every 36 deg. Unreachable: latitude -30 at longitudes 324 and 0 (far side, below).
// In the A5 > 0 wrist A4 follows the stylus azimuth and is +-180 deg where the stylus points away from
// the robot (longitude 162.5 deg). Latitudes 30 and 60 are closed rings: tilting the stylus towards
// straight down near the robot keeps A4 within +-70 deg there. Latitudes 0 and -30 span A4 from -162
// to +161 deg and must not cross longitude 162.5 deg: they are scanned from longitude 180 around the
// far side to 330, then the upper rings and the pole, then from longitude 0 to 150. Tilts keep
// 51 deg <= |A5| <= 115 deg; everything stays in one wrist configuration. On the low touches the
// tilt also lifts the A4/A5 wrist housings, which hang below the stylus when it points upwards: each
// 15 deg raises them by about 50 mm, to at least 65 mm above the stand's table (scratch estimate).
constexpr std::array<TouchRow, ProbeTouchCount> Touches{{
    {0, 180, 0, 30}, {-30, 180, 30, 30}, {-30, 216, 30, 0}, {0, 210, 0, 0}, {0, 240, 0, 0},
    {-30, 252, 30, 0}, {-30, 288, 50, 0}, {0, 270, 0, 0}, {0, 300, 40, 30}, {0, 330, 45, 60},
    {30, 342, 30, 60}, {30, 18, 15, 90}, {30, 54, 0, 120}, {30, 90, 0, 120}, {30, 126, 50, -150},
    {30, 162, 50, -120}, {30, 198, 50, -90}, {30, 234, 0, 0}, {30, 270, 0, 0}, {30, 306, 15, 30},
    {60, 300, 0, 30}, {60, 0, 0, 90}, {60, 60, 0, 150}, {60, 120, 15, -150}, {60, 180, 30, -90},
    {60, 240, 0, 0},
    {90, 0, 0, 90},
    {0, 0, 45, 90}, {0, 30, 30, 120}, {-30, 36, 55, 120}, {0, 60, 15, 120}, {-30, 72, 45, 120},
    {-30, 108, 45, 90}, {0, 90, 15, 90}, {0, 120, 15, 90}, {-30, 144, 30, 60}, {0, 150, 0, 60}}};

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
      * makeRotation(180.0, Axis::X) * makeRotation(row.roll, Axis::Z);
  return rotation.block<3, 3>(0, 0);
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
    const double clearance = std::min(parameters.strutClearance(touch.start.block<3, 1>(0, 3)),
                                      parameters.strutClearance(touch.contact.block<3, 1>(0, 3)));
    if (clearance < ProbeApproachGap)
      return {{}, QStringLiteral("Touch %1/%2: strut clearance %3 mm is below %4 mm.")
                      .arg(index + 1).arg(ProbeTouchCount).arg(clearance, 0, 'f', 4).arg(ProbeApproachGap)};
    plan.touches.append(touch);
  }
  const ProbeTouch& first = plan.touches.front();
  const ProbeTouch& last = plan.touches.back();
  plan.entry = makeTransform(first.start.block<3, 3>(0, 0),
                             centre + EntryDistance * first.normal + EntryRise * V3d::UnitZ());
  plan.exit = makeTransform(last.start.block<3, 3>(0, 0), centre + ExitDistance * last.normal);
  return {std::move(plan), {}};
}
