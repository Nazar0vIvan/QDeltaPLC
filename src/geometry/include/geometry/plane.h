#pragma once

#include <optional>

#include <QVector>

#include "mathtypes.h"

struct Plane {
  V4d coeffs{0.0, 0.0, 1.0, 0.0};

  // Orthogonal least squares. The largest-magnitude normal component is positive;
  // exact ties prefer X, then Y, then Z. eps is the relative rank tolerance.
  static std::optional<Plane> fromPoints(const QVector<V3d>& points, double eps = GeomConst::Eps);

  V3d normal() const noexcept;
};
