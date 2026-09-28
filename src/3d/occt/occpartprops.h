#pragma once

#include <gp_Trsf.hxx>
#include <Quantity_Color.hxx>

#include <QString>
#include <algorithm>

namespace RoboCrap3D {

// Three byte-valued color channels form one RGB color.
inline Quantity_Color rgb(int r, int g, int b)
{
  return Quantity_Color(
      static_cast<double>(std::clamp(r, 0, 255)) / 255.0,
      static_cast<double>(std::clamp(g, 0, 255)) / 255.0,
      static_cast<double>(std::clamp(b, 0, 255)) / 255.0,
      Quantity_TOC_RGB);
}

inline const Quantity_Color kDefaultPartColor{0.72, 0.76, 0.80, Quantity_TOC_RGB};
inline const Quantity_Color kRoughSurfaceColor{80.0 / 255.0, 157.0 / 255.0,
                                               253.0 / 255.0, Quantity_TOC_RGB};

enum class OccSelectionMode { PartOnly, All, None };

struct OccPartProps
{
  gp_Trsf transform;
  Quantity_Color color = kDefaultPartColor;
  OccSelectionMode selectionMode = OccSelectionMode::None;
  bool wireframe = false;
  bool topmost = false;
  double markerSize = 0.0;
  double lineWidth = 0.0;
  bool showTrihedron = false;
  double trihedronSize = 60.0;
};

} // namespace RoboCrap3D
