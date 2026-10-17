#include "robotdefinition.h"

#include "geometry/utils.h"

namespace RoboCrap3D {

namespace {

// One joint: offset to the preceding frame (mm, Z-Y-X degrees), signed local Z axis,
// position limits (degrees) and rated-payload speed (degrees per second).
struct JointRow
{
  double dx, dy, dz, da, db, dc;
  double axisZ;
  double qMin, qMax;
  double ratedSpeed;
};

constexpr std::array<JointRow, DofCount> Joints{{
  {0.0, 0.0, 227.0, 0.0, 0.0, 0.0, -1.0, -170.0, 170.0, 220.0},
  {150.0, -90.4, 223.0, 0.0, 0.0, -90.0, 1.0, -185.0, 65.0, 210.0},
  {0.0, -610.0, -11.1, 90.0, 0.0, 0.0, 1.0, -137.0, 163.0, 270.0},
  {-20.0, -357.0, 101.5, 0.0, 0.0, -90.0, 1.0, -185.0, 185.0, 381.0},
  {0.0, -39.0, -303.0, 0.0, 0.0, 90.0, 1.0, -120.0, 120.0, 311.0},
  {0.0, -80.0, -39.0, 0.0, 0.0, 90.0, -1.0, -350.0, 350.0, 492.0},
}};

} // namespace

RobotDefinition RobotDefinition::kr10()
{
  RobotDefinition result;
  for (std::size_t i = 0; i < DofCount; ++i) {
    const JointRow& row = Joints[i];
    JointModel& joint = result.kinematics.joints[i];
    joint.axis = V3d{0.0, 0.0, row.axisZ};
    joint.localTransform = makeTranslation(V3d{row.dx, row.dy, row.dz})
        * makeRotation(row.da, V3d{0.0, 0.0, 1.0})
        * makeRotation(row.db, V3d{0.0, 1.0, 0.0})
        * makeRotation(row.dc, V3d{1.0, 0.0, 0.0});
    joint.qMin = row.qMin;
    joint.qMax = row.qMax;
    joint.ratedPayloadSpeedDegreesPerSecond = row.ratedSpeed;
  }
  result.kinematics.ik = {150.0, 450.0, 610.0, 20.0, 660.0, 80.0};
  result.kinematics.qHome = V6d{0.0, -90.0, 90.0, 0.0, 0.0, 0.0};
  result.kinematics.pHome = V6d{890.0, 0.0, 1080.0, 0.0, 90.0, 0.0};
  const std::array<int, 3> black{0, 0, 0};
  const std::array<int, 3> orange{255, 106, 0};
  result.visuals.links = {
    {QStringLiteral("0_BASE.brep"), black},
    {QStringLiteral("1_ROTATING_COLUMN.brep"), orange},
    {QStringLiteral("2_LINK_ARM.brep"), orange},
    {QStringLiteral("3_IN-LINE_WRIST.brep"), orange},
    {QStringLiteral("4_WRIST1.brep"), orange},
    {QStringLiteral("5_WRIST2.brep"), orange},
    {QStringLiteral("6_WRIST3.brep"), black},
  };
  return result;
}

} // namespace RoboCrap3D
