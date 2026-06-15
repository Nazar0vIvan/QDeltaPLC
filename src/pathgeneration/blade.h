#pragma once

#include <QString>
#include <QFile>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>

#include "../geometry/utils.h"
#include "../geometry/pose.h"

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

Pose getCxCvStartFrenet(const QVector<V3d>& cx, double L, const Pose& frenet);
Pose getCxCvEndFrenet(const QVector<V3d>& cx, double L, const Pose& frenet);
Pose getCxCvFrenet(V3d pt, const V3d& poly, const V3d& v0);
QVector<Pose> getCxCvFrenets(const QVector<V3d>& cx, const QVector<V3d>& cx_next, double L);




