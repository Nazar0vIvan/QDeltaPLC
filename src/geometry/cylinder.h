#pragma once

#include <QVector>

#include <optional>

#include "pose.h"

class Cylinder
{
public:
  Cylinder() = default;

  static std::optional<Cylinder> fromAxis(const V3d& axisDir, const V3d& origin, double radius, Axis axis = Axis::X);

  void setSurfacePose(double offset, double angleDeg);

  QVector<Pose> surfaceRing(int n, double offset) const;

  Pose originPose() const;
  Pose surfacePose() const;
  V3d axisDir() const;
  double radius() const;

  Axis axis() const;

private:
  V3d m_axisDir{V3d::Zero()};
  Pose m_originPose{};
  double m_radius = 0.0;
  Axis m_axis{Axis::X};
  Pose m_surfacePose{};
};


