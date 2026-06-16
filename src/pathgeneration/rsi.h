#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "geometry/pose.h"

struct MotionParams {
  double v{0.0};
  double a{0.0};
};

struct MotionProfile {
  double v{0.0};
  double a{0.0};
  double tAcc{0.0};
  double sAcc{0.0};
  double sConst{0.0};
  double tConst{0.0};
  double totalTime{0.0};
};

class RsiPath
{
public:
  RsiPath() = default;

  static std::optional<QVector<Pose>> fromSurfPoses(const QVector<Pose>& surfPoses, const M4d& aiT);

  QVector<V6d> lin(const V6d& p1, const V6d& p2, const MotionParams& mp, int decimals = 3);

  QVector<V6d> polyline(const QVector<V6d>& refPoints, const MotionParams& mp, int decimals = 3);
};

bool writeOffsetsToJson(const QVector<V6d>& offsets, const QString& filePath, int decimals = 3);
