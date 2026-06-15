#pragma once

#include <optional>

#include "mathtypes.h"

struct Plane {
  V4d coeffs{0.0, 0.0, 1.0, 0.0};

  static std::optional<Plane> fromPoints(const VXdRef& x, const VXdRef& y, const VXdRef& z, double eps = GeomConst::Eps);

  V3d normal() const noexcept;
  std::optional<V3d> explicitCoeffs(double eps = GeomConst::Eps) const noexcept;
};
