#pragma once

#include "mathtypes.h"

#include <QVector>
#include <optional>

struct SphereFit
{
  V3d center = V3d::Zero();
  double radius = 0.0;
  double rms = 0.0; // Root mean square of the radial residuals ||p - center|| - radius.
};

// Algebraic least-squares sphere through at least four noncoplanar finite points.
std::optional<SphereFit> fitSphere(const QVector<V3d>& points);
