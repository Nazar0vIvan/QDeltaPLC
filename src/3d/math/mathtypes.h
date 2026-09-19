#pragma once

#include <array>
#include <vector>

#include <gp_Mat.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

namespace RoboCrap3D {

namespace GeomConst {
  constexpr double Eps = 1e-9;
  constexpr double PosEps = 1e-4;
  constexpr double RotEps = 1e-6;
  constexpr double Pi = 3.141592653589793238462643383279502884;
  constexpr double DegToRad = Pi / 180.0;
  constexpr double RadToDeg = 180.0 / Pi;
}

using VXd = std::vector<double>;
using V6d = std::array<double, 6>;
using V4d = std::array<double, 4>;
using V3d = gp_Vec;
using M3d = gp_Mat;
using M4d = gp_Trsf;

struct EulerSolution
{
  double A1{}; double A2{};
  double B1{}; double B2{};
  double C1{}; double C2{};
};

struct OrthoBasis
{
  V3d e1{1.0, 0.0, 0.0};
  V3d e2{0.0, 1.0, 0.0};
  V3d e3{0.0, 0.0, 1.0};
};

} // namespace RoboCrap3D
