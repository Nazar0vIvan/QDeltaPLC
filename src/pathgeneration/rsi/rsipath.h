#pragma once

#include <optional>

#include <QVector>

#include "geometry/pose.h"
#include "pathgeneration/rsi/motionprofile.h"

class RsiPath
{
public:
  RsiPath() = default;

  static std::optional<Pose> fromSurfPose(const Pose& surfPose, const M4d& aiS);
  static std::optional<QVector<Pose>> fromSurfPoses(const QVector<Pose>& surfPoses, const M4d& aiS);

  static QVector<V6d> lin(const V6d& p1, const V6d& p2, const MotionParams& mp, int decimals = 3);
  static QVector<V6d> polyline(const QVector<V6d>& refPoints, const MotionParams& mp, int decimals = 3);
};
