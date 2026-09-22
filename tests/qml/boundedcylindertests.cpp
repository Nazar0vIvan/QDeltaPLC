#include "geometry/boundedcylinder.h"
#include "geometry/planeimporter.h"
#include "scene/cylindergeometry.h"
#include "scene/scenemodel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QTemporaryFile>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

using Point = BoundedCylinder::Point;

void require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

bool near(double a, double b, double tolerance = 1e-3)
{
  return std::abs(a - b) <= tolerance;
}

std::vector<Point> sample()
{
  return {
      {919.610168, -58.9764290, 643.898193},
      {925.572510, -45.0691261, 638.017822},
      {920.250427, -38.7094193, 624.221802},
      {932.792847, -42.0696000, 613.405701},
      {926.593689, -53.4439316, 604.491821},
      {931.871155, -73.5837402, 637.811646},
      {926.282532, -78.7271000, 620.813232},
      {920.361816, -68.4017792, 606.092000},
      {933.397217, -69.1570206, 606.341553},
  };
}

std::vector<Point> idealCylinder(bool oneRing = false, bool narrowArc = false)
{
  const Point center{1500.0, -300.0, 700.0};
  const double inv = 1.0 / std::sqrt(14.0);
  const Point axis{inv, 2.0 * inv, 3.0 * inv};
  const Point tangent{-2.0 / std::sqrt(5.0), 1.0 / std::sqrt(5.0), 0.0};
  const Point second{
      axis[1] * tangent[2] - axis[2] * tangent[1],
      axis[2] * tangent[0] - axis[0] * tangent[2],
      axis[0] * tangent[1] - axis[1] * tangent[0]};
  std::vector<Point> result;
  for (int station = 0; station < 3; ++station) {
    for (int ring = 0; ring < 8; ++ring) {
      const double angle = (narrowArc ? 0.01 : 2.0 * 3.141592653589793) * ring / 8.0;
      const double axial = oneRing ? 0.0 : (station - 1) * 40.0;
      Point point{};
      for (int i = 0; i < 3; ++i)
        point[i] = center[i] + axial * axis[i]
                   + 12.0 * (std::cos(angle) * tangent[i] + std::sin(angle) * second[i]);
      result.push_back(point);
    }
  }
  return result;
}

void run()
{
  const auto points = sample();
  const auto cylinder = BoundedCylinder::fromPoints(points);
  require(cylinder.has_value(), "Measured point fit failed");
  const QString fixtureRoot = QStringLiteral(ROBOCRAP_SOURCE_DIR "/resources/json/");
  const auto imported = BoundedCylinder::fromJsonFile(fixtureRoot + QStringLiteral("rough-cylinder-sample.json"));
  require(imported && imported->points == points, "Valid cylinder JSON fixture failed");
  require(!BoundedCylinder::fromJsonFile(fixtureRoot + QStringLiteral("rough-cylinder-invalid.json")),
          "Invalid cylinder JSON fixture was accepted");
  require(cylinder->points == points, "Original samples changed");
  require(near(cylinder->radius, 20.043646, 0.1), "Incorrect measured radius");
  require(near(cylinder->axis[0], 0.999349, 0.01)
          && near(cylinder->axis[1], -0.036055, 0.01)
          && near(cylinder->axis[2], 0.000879, 0.01), "Incorrect measured axis");
  require(cylinder->rmsResidual < 0.1, "Measured residual too large");
  require(cylinder->length > 20.0 && cylinder->length < 50.0, "Incorrect finite length");

  auto moved = points;
  for (auto& point : moved) { point[0] += 1500; point[1] -= 200; point[2] += 75; }
  const auto translated = BoundedCylinder::fromPoints(moved);
  require(translated && near(translated->radius, cylinder->radius, 1e-4),
          "Translation changed fitted radius");
  for (int i = 0; i < 3; ++i)
    require(near(translated->axis[i], cylinder->axis[i], 1e-4), "Translation changed axis");

  const auto ideal = BoundedCylinder::fromPoints(idealCylinder());
  require(ideal && near(ideal->radius, 12.0) && near(ideal->length, 80.0),
          "Ideal rotated cylinder failed");
  require(near(ideal->origin[0], 1500.0) && near(ideal->origin[1], -300.0)
          && near(ideal->origin[2], 700.0), "Incorrect axis midpoint");

  require(!BoundedCylinder::fromPoints({}), "Accepted empty input");
  require(!BoundedCylinder::fromPoints(std::vector<Point>(points.begin(), points.begin() + 5)),
          "Accepted fewer than six points");
  auto invalid = points;
  invalid[0][0] = std::numeric_limits<double>::quiet_NaN();
  require(!BoundedCylinder::fromPoints(invalid), "Accepted nonfinite coordinate");
  require(!BoundedCylinder::fromPoints({{0,0,0},{1,0,0},{2,0,0},{3,0,0},{4,0,0},{5,0,0}}),
          "Accepted collinear points");
  require(!BoundedCylinder::fromPoints(idealCylinder(true)), "Accepted a single axial ring");
  require(!BoundedCylinder::fromPoints(idealCylinder(false, true)), "Accepted a narrow arc");

  QTemporaryFile file;
  require(file.open(), "Cannot create temporary JSON fixture");
  file.write("[[1,2,3],[4,5]]");
  file.flush();
  require(!BoundedCylinder::fromJsonFile(file.fileName()), "Accepted malformed point JSON");

  SceneModel scene;
  PlaneImporter importer;
  SceneObject* loaded = nullptr;
  int failures = 0;
  QObject::connect(&importer, &PlaneImporter::loaded, [&loaded](SceneObject* object) { loaded = object; });
  QObject::connect(&importer, &PlaneImporter::failed, [&failures](const QString&) { ++failures; });
  auto finishImport = [&importer]() {
    QElapsedTimer timer;
    timer.start();
    while (importer.busy() && timer.elapsed() < 10000) QCoreApplication::processEvents();
    require(!importer.busy(), "Import worker did not finish");
  };
  const QUrl source = QUrl::fromLocalFile(fixtureRoot + QStringLiteral("rough-cylinder-sample.json"));
  importer.loadCylinder(source, &scene);
  finishImport();
  require(failures == 0 && loaded, "Cylinder import did not create a scene object");
  require(loaded->kind() == SceneObject::Cylinder && loaded->classification() == SceneObject::Rough,
          "Wrong cylinder scene classification");
  require(loaded->sourceUrl() == source && scene.findObject(loaded->objectId()) == loaded,
          "Cylinder source or identity was lost");
  auto* geometry = qobject_cast<CylinderGeometry*>(loaded->geometry());
  require(geometry && geometry->parent() == loaded && geometry->pointCount() == 9,
          "Cylinder geometry is not owned by the scene object");
  require(near(geometry->radius(), cylinder->radius, 1e-4)
          && near(geometry->length(), cylinder->length, 1e-4),
          "Cylinder Properties disagree with the fitted data");
  const QString firstId = loaded->objectId();
  loaded = nullptr;
  importer.loadCylinder(source, &scene);
  finishImport();
  require(failures == 0 && loaded && loaded->objectId() != firstId && scene.objectList().size() == 2,
          "Repeated cylinder imports did not remain independent");
  require(scene.setObjectVisible(loaded, false)
          && scene.findObject(firstId)->visible() && !loaded->visible(),
          "Cylinder visibility leaked between imports");

  loaded = nullptr;
  importer.loadCylinder(QUrl::fromLocalFile(fixtureRoot + QStringLiteral("rough-cylinder-invalid.json")), &scene);
  finishImport();
  require(failures == 1 && !loaded && scene.objectList().size() == 2,
          "Invalid cylinder import created a partial scene object");

  loaded = nullptr;
  importer.load(QUrl::fromLocalFile(fixtureRoot + QStringLiteral("rough-plane-sample.json")), &scene);
  finishImport();
  require(failures == 1 && loaded && loaded->kind() == SceneObject::Plane,
          "Plane import regressed after sharing the worker");
}

} // namespace

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  try {
    run();
    qInfo() << "Bounded cylinder regressions passed";
    return 0;
  } catch (const std::exception& error) {
    qCritical() << error.what();
    return 1;
  }
}
