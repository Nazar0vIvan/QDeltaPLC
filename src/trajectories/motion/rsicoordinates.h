#pragma once

#include "geometry/mathtypes.h"

#include <cmath>

// Coordinates written to the KR C4 (RSI corrections and KRL job files).

// Principal KUKA A, B, C (Z-Y-X, degrees) of a rotation: B in [-90, 90]; C = 0 at B = +/-90.
inline V3d principalEulerDegrees(const M3d& rotation)
{
  // atan2 retains pitch accuracy near +/-90 degrees, where asin can lose it.
  const double cosinePitch = std::hypot(rotation(0, 0), rotation(1, 0));
  if (cosinePitch <= GeomConst::Eps)
    return {std::atan2(-rotation(0, 1), rotation(1, 1)) * GeomConst::RadToDeg,
            std::copysign(90.0, -rotation(2, 0)), 0.0};
  return {std::atan2(rotation(1, 0), rotation(0, 0)) * GeomConst::RadToDeg,
          std::atan2(-rotation(2, 0), cosinePitch) * GeomConst::RadToDeg,
          std::atan2(rotation(2, 1), rotation(2, 2)) * GeomConst::RadToDeg};
}

// The value the controller parses from six fixed decimal places.
inline V6d wireValues(const V6d& values)
{
  constexpr double WireScale = 1e6;
  return ((values * WireScale).array().round() / WireScale).matrix();
}
