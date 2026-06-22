#pragma once

#include <optional>

#include <QVector>

#include "geometry/pose.h"

std::optional<Pose> getCxCvStartFrenet(const QVector<V3d>& cx, double length, const Pose& frenet);

std::optional<Pose> getCxCvEndFrenet(const QVector<V3d>& cx,  double length, const Pose& frenet);

std::optional<Pose> getCxCvFrenet(const V3d& point, const V3d& coeffs, const V3d& v0);

std::optional<QVector<Pose>> getCxCvFrenets(const QVector<V3d>& cx, const QVector<V3d>& cxNext, double length);