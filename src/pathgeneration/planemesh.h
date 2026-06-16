#pragma once

#include <optional>

#include <QVector>

#include "geometry/plane.h"
#include "geometry/pose.h"

struct PlaneMeshAxis {
  V3d dir{V3d::Zero()};
  double step{0.0};
  int count{0};
};

struct PlanePoseMeshParams {
  Plane plane{};
  V3d origin{V3d::Zero()};

  PlaneMeshAxis axis1{};
  PlaneMeshAxis axis2{};
};

struct PoseMesh {
  QVector<QVector<Pose>> poses;

  bool empty() const noexcept;
  int rows() const noexcept;
  int cols() const noexcept;
  int size() const noexcept;

  Pose& at(int row, int col);
  const Pose& at(int row, int col) const;
};

std::optional<PoseMesh> makePlanePoseMesh(const PlanePoseMeshParams& params);
