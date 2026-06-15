#pragma once

#include <QVector>

#include "../geometry/utils.h"
#include "../geometry/plane.h"
#include "../geometry/pose.h"

struct PlaneMeshAxis {
  V3d dir{V3d::Zero()}; // must belong to plane
  double step{0.0};     // signed step, mm
  int count{0};
};

struct PlanePoseMeshParams {
  Plane plane{};
  V3d origin{V3d::Zero()};

  PlaneMeshAxis axis1{};
  PlaneMeshAxis axis2{};

  bool projectOriginToPlane{true};
};

struct PoseMesh {
  QVector<QVector<Pose>> poses; // poses[row][col]

  bool empty() const noexcept;
  int rows() const noexcept;
  int cols() const noexcept;
  int size() const noexcept;

  Pose& at(int row, int col);
  const Pose& at(int row, int col) const;
};

PoseMesh makePlanePoseMesh(const PlanePoseMeshParams& params);
