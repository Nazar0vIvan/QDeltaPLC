#pragma once

#include <Eigen/Core>
#include <optional>

#include "utils.h"

struct Plane {
  V4d coeffs{0.0, 0.0, 1.0, 0.0};

  static std::optional<Plane> fromPoints(
    const Eigen::Ref<const Eigen::VectorXd>& x,
    const Eigen::Ref<const Eigen::VectorXd>& y,
    const Eigen::Ref<const Eigen::VectorXd>& z,
    double eps = GeomConst::Eps
  );

  V3d normal() const noexcept;
  std::optional<V3d> explicitCoeffs() const noexcept;
};
