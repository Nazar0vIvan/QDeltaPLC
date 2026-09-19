#pragma once

#include "3d/robot/model/kr10model.h"
#include "3d/viewportassets.h"

#include <QThread>
#include <TopoDS_Shape.hxx>

namespace RoboCrap3D {

struct CadLoadResult
{
  std::array<TopoDS_Shape, LinkCount> links;
  TopoDS_Shape endEffector;
  QString error;
  QString warning;
};

// Only file/shape work runs here. The result is read after finished + wait().
class CadLoadWorker final : public QThread
{
public:
  CadLoadWorker(Kr10Model model, ViewportAssets assets);
  CadLoadResult takeResult();

protected:
  void run() override;

private:
  Kr10Model m_model;
  ViewportAssets m_assets;
  CadLoadResult m_result;
};

} // namespace RoboCrap3D
