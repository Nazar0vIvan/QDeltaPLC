#pragma once

#include <QVector>

#include "pose.h"

class Cylinder
{
public:
  Cylinder() = default;

  static std::optional<Cylinder> fromTwoPoints(const V3d& c1, const V3d& c2, double radius, Axis axis = Axis::X);
  static std::optional<Cylinder> fromAxis(const V3d& axisDir, double radius, Axis axis = Axis::X);

  void setOriginPose(const V3d& origin);
  void setSurfacePose(double offset, double angleDeg);

  QVector<Pose> surfaceRing(int n, double offset) const;

  Pose originPose() const;
  Pose surfacePose() const;
  V3d axisDir() const;
  double radius() const;

private:
  Pose m_originPose{};
  Pose m_surfacePose{};
  V3d m_axisDir{V3d::Zero()};
  double m_R = 0.0;
};


