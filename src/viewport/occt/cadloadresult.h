#pragma once

#include <QList>
#include <QString>
#include <TopoDS_Shape.hxx>

namespace RoboCrap3D {

struct CadLoadResult
{
  QList<TopoDS_Shape> shapes;
  QString error;
};

} // namespace RoboCrap3D
