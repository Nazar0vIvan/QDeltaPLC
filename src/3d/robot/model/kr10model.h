#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include <QString>

#include "3d/math/mathtypes.h"
#include "3d/robot/model/jointmodel.h"
#include "3d/robot/model/linkmodel.h"

namespace RoboCrap3D {

constexpr std::size_t DofCount = 6;
constexpr std::size_t LinkCount = DofCount + 1;
constexpr std::size_t EndEffIdx = LinkCount - 1;

struct Kr10Model
{
  struct IkParams
  {
    double sx = 0.0;
    double sz = 0.0;
    double a = 0.0;
    double bx = 0.0;
    double by = 0.0;
    double dF = 0.0;
  };

  QString name;

  std::array<JointModel, DofCount> joints{};
  std::array<LinkModel, LinkCount> links{};

  LinkModel endEffector{};

  IkParams ik{};

  V6d qHome{};
  V6d pHome{};

  static std::optional<Kr10Model> fromJson(const QString& file);
};
} // namespace RoboCrap3D
