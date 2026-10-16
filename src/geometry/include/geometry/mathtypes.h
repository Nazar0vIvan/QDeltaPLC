#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace GeomConst {
  constexpr double Eps = 1e-9;
  constexpr double Pi = 3.141592653589793238462643383279502884;
  constexpr double DegToRad = Pi / 180.0;
  constexpr double RadToDeg = 180.0 / Pi;
}

enum class Axis { X, Y, Z };

using V6d = Eigen::Matrix<double, 6, 1>;
using V3d = Eigen::Vector3d;
using V4d = Eigen::Vector4d;
using M3d = Eigen::Matrix3d;
using M4d = Eigen::Matrix4d;
using VXd = Eigen::VectorXd;
using VXdRef = Eigen::Ref<const VXd>;

struct EulerSolution {
  double A1{}; double A2{};
  double B1{}; double B2{};
  double C1{}; double C2{};
};

struct OrthoBasis {
  V3d e1{V3d::UnitX()};
  V3d e2{V3d::UnitY()};
  V3d e3{V3d::UnitZ()};
};
