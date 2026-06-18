#pragma once

#include <optional>

#include <QVector>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

#include "mathtypes.h"

struct Plane {
  V4d coeffs{0.0, 0.0, 1.0, 0.0};

  static std::optional<Plane> fromPoints(const QVector<V3d>& points, double eps = GeomConst::Eps);
  static std::optional<Plane> fromJsonFile(const QString& jsonFilePath, double eps = GeomConst::Eps);

  V3d normal() const noexcept;
  std::optional<V3d> explicitCoeffs(double eps = GeomConst::Eps) const noexcept;
};
