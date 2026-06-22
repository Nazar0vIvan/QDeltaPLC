#pragma once

#include <QVector>

#include "geometry/mathtypes.h"

struct BladeProfile
{
  QVector<V3d> cx;
  QVector<V3d> cv;
  QVector<V3d> re;
  QVector<V3d> le;
};

using Airfoil = QVector<BladeProfile>;