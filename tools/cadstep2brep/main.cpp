// Build-time converter: cadstep2brep <input.stp> <output.brep>.
#include <BRepTools.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>

#include <exception>
#include <fstream>
#include <iostream>
#include <string>

namespace {

struct StepReadResult
{
  TopoDS_Shape shape;
  std::string error;
};

const char* statusName(IFSelect_ReturnStatus status)
{
  switch (status) {
    case IFSelect_RetVoid: return "IFSelect_RetVoid";
    case IFSelect_RetDone: return "IFSelect_RetDone";
    case IFSelect_RetError: return "IFSelect_RetError";
    case IFSelect_RetFail: return "IFSelect_RetFail";
    case IFSelect_RetStop: return "IFSelect_RetStop";
  }
  return "unknown status";
}

// Assembly placements are applied by TransferRoots, so the shape is in the file's top-level coordinates.
StepReadResult readStep(const char* path)
{
  STEPControl_Reader reader;
  const IFSelect_ReturnStatus status = reader.ReadFile(path);
  if (status != IFSelect_RetDone) return {{}, std::string("cannot read ") + path + ": " + statusName(status)};
  if (reader.TransferRoots() <= 0) return {{}, std::string("no transferable roots in ") + path};
  const TopoDS_Shape shape = reader.OneShape();
  if (shape.IsNull()) return {{}, std::string("empty shape in ") + path};
  return {shape, {}};
}

std::string writeBrep(const TopoDS_Shape& shape, const char* path)
{
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  BRepTools::Write(shape, stream);
  stream.flush();
  return stream ? std::string() : std::string("cannot write ") + path;
}

} // namespace

int main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cerr << "usage: cadstep2brep <input.stp> <output.brep>\n";
    return 2;
  }
  std::string error;
  try {
    const StepReadResult read = readStep(argv[1]);
    error = read.error.empty() ? writeBrep(read.shape, argv[2]) : read.error;
  } catch (const Standard_Failure& failure) {
    error = failure.what();
  } catch (const std::exception& failure) {
    error = failure.what();
  }
  if (!error.empty()) {
    std::cerr << "cadstep2brep: " << error << '\n';
    return 1;
  }
  std::cout << "cadstep2brep: " << argv[1] << " -> " << argv[2] << '\n';
  return 0;
}
