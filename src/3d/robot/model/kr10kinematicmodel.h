#pragma once

#include "geometry/mathtypes.h"

#include <array>
#include <cstddef>
#include <limits>

namespace RoboCrap3D {

constexpr std::size_t DofCount = 6;
constexpr std::size_t LinkCount = DofCount + 1;
constexpr std::size_t EndEffIdx = LinkCount - 1;

// KR10 pose acceptance tolerances, in position units and rotation-matrix entries.
constexpr double Kr10PosEps = 1e-4;
constexpr double Kr10RotEps = 1e-6;

// Temporary offline diagnostic: bypass A1-A6 position limits in IK, chamfer
// generation/evaluation and preview FK. Set false to restore enforcement.
// Model limits remain intact for seed selection; device control does not use this.
constexpr bool IgnorePreviewJointPositionLimits = true;

struct JointModel
{
  V3d axis{0.0, 0.0, 1.0};

  // Transform from the current joint coordinate system to the previous one.
  M4d localTransform = M4d::Identity();

  double qMin = -std::numeric_limits<double>::infinity();
  double qMax =  std::numeric_limits<double>::infinity();
};

struct Kr10KinematicModel
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

  std::array<JointModel, DofCount> joints{};
  IkParams ik{};
  V6d qHome = V6d::Zero();
  V6d pHome = V6d::Zero();
};

} // namespace RoboCrap3D
