#pragma once

#include <optional>

#include <QString>

#include "kr10kinematicmodel.h"
#include "3d/occt/occpartprops.h"

namespace RoboCrap3D {

struct LinkModel
{
  QString fileName;
  OccPartProps props{};
};

struct RobotVisualModel
{
  std::array<LinkModel, LinkCount> links{};
};

struct Kr10Model
{
  QString name;
  Kr10KinematicModel kinematics;
  RobotVisualModel visuals;

  static std::optional<Kr10Model> fromJson(const QString& file);
};
} // namespace RoboCrap3D
