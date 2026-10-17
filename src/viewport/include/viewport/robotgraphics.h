#pragma once

#include "geometry/mathtypes.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <array>
#include <optional>

namespace RoboCrap3D {

enum class RobotTool { Measuring, Spindle };

struct LinkModel
{
  QString fileName;
  std::array<int, 3> color{};
};

struct RobotVisualModel
{
  QList<LinkModel> links;
  QStringList tools; // Absolute CAD paths in RobotTool order, loaded with the links.
};

struct ViewportOperationResult
{
  bool accepted = false;
  QString error;
};

} // namespace RoboCrap3D
