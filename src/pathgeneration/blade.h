#pragma once

#include <QString>
#include <QFile>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>

#include "geometry/pose.h"

struct BladeProfile {
  QVector<V3d> cx, cv, re, le;
};

using Airfoil = QVector<BladeProfile>;

class BladeJsonLoader
{
public:
  struct LoadResult {
    QString path;
    Airfoil airfoil;
    bool ok = false;
    QString error;
  };

  // Accepts the same input as SocketRSI::loadBladeJson (expects {"path": QUrl})
  static LoadResult load(const QVariantMap& data);

  // Direct file-path entry point (useful for non-QML callers/tests)
  static LoadResult loadFromFile(const QString& path);
};

std::optional<Pose> getCxCvStartFrenet(const QVector<V3d>& cx, double length, const Pose& frenet);
std::optional<Pose> getCxCvEndFrenet(const QVector<V3d>& cx, double length, const Pose& frenet);
std::optional<Pose> getCxCvFrenet(const V3d& point, const V3d& poly2d, const V3d& v0);
std::optional<QVector<Pose>> getCxCvFrenets(const QVector<V3d>& cx, const QVector<V3d>& cxNext, double length);




