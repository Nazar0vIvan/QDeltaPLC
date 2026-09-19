#pragma once

#include <gp_Trsf.hxx>
#include <Quantity_Color.hxx>

#include <QString>

namespace RoboCrap3D {

inline const Quantity_Color kDefaultPartColor{0.72, 0.76, 0.80, Quantity_TOC_RGB};

enum class OccSelectionMode { PartOnly, All, None };

struct OccPartProps
{
  gp_Trsf transform;
  Quantity_Color color = kDefaultPartColor;
  OccSelectionMode selectionMode = OccSelectionMode::None;
  bool showTrihedron = false;
  double trihedronSize = 60.0;
};

} // namespace RoboCrap3D
