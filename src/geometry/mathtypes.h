#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

enum class Axis { X, Y, Z };

using V6d = Eigen::Matrix<double, 6, 1>;
using V3d = Eigen::Vector3d;
using V4d = Eigen::Vector4d;
using M3d = Eigen::Matrix3d;
using M4d = Eigen::Matrix4d;

struct EulerSolution {
  double A1{}; double A2{};
  double B1{}; double B2{};
  double C1{}; double C2{};
};

struct Basis {
  V3d x{V3d::UnitX()};
  V3d y{V3d::UnitY()};
  V3d z{V3d::UnitZ()};
};
