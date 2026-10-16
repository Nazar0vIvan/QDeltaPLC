#pragma once

#include <optional>

#include <QString>

#include "kinematics/kr10kinematicmodel.h"
#include "viewport/robotgraphics.h"

namespace RoboCrap3D {

struct RobotDefinition
{
  QString name;
  Kr10KinematicModel kinematics;
  RobotVisualModel visuals;

  static std::optional<RobotDefinition> fromJson(const QString& file);
};
} // namespace RoboCrap3D
