#pragma once

#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>

#include "../geometry/utils.h"
#include "../geometry/pose.h"

struct MotionParams {
  double v, a;
};

class RsiPath
{
public:
  RsiPath();

  static QVector<Pose> fromSurfPoses(const QVector<Pose>& surf_poses, const M4d& AiT);

  QVector<V6d> lin(const V6d& P1, const V6d& P2, const MotionParams& mp, int decimals = 3);
  QVector<V6d> polyline(const QVector<V6d> &ref_points, const MotionParams& mp, int decimals = 3);

};

void writeOffsetsToJson(const QVector<V6d>& offsets, const QString& filePath, int decimals = 3);
