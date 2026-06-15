#pragma once

#include <QVector>

#include "utils.h"
#include "pose.h"

struct Cylinder
{
public:
  static Cylinder fromTwoPoints(const V3d& c1, const V3d& c2, const V3d& pc,
                                double R, double L,
                                char axis = 'y');

  static Cylinder fromAxis(const V3d& u,
                           const V3d& pc,
                           double R, double L,
                           char axis = 'y');

  Pose surfacePose(char axis1, double val1,
                   char axisRot, double angleDeg,
                   char axis2, double val2,
                   bool returnLocal = false) const;

  QVector<Pose> surfaceRing(int n, double L) const;

  double R;
  double L;
  Pose pose;
};


