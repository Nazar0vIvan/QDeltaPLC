#pragma once

#include "boundedplane.h"
#include "boundedcylinder.h"

#include <optional>

// Analytic intersection geometry, independent of scene ownership and rendering.
class EdgeGeometry
{
public:
  struct Ellipse {
    V3d center = V3d::Zero();
    V3d majorAxis = V3d::UnitX();
    V3d minorAxis = V3d::UnitY();
    double majorRadius = 0.0;
    double minorRadius = 0.0;
    bool isCircle() const { return majorRadius == minorRadius; }
  };

  static std::optional<EdgeGeometry> fromEllipse(Ellipse ellipse);
  const Ellipse& ellipse() const { return m_ellipse; }

private:
  explicit EdgeGeometry(Ellipse ellipse);
  Ellipse m_ellipse;
};

enum class IntersectionStatus {
  Success,
  ParallelCylinderAxis,
  InvalidGeometry
};

struct IntersectionResult {
  IntersectionStatus status = IntersectionStatus::InvalidGeometry;
  std::optional<EdgeGeometry> edge;
};

IntersectionResult intersectSurfaces(const BoundedPlane& plane, const BoundedCylinder& cylinder);
