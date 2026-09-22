#include "geometry/boundedplane.h"
#include "geometry/planeimporter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QTemporaryFile>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

using Point = BoundedPlane::Point;

void require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

double dot(const Point& a, const Point& b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

bool near(double a, double b) { return std::abs(a - b) < 1e-7; }

void verifyRectangle(const BoundedPlane& plane, const std::vector<Point>& samples)
{
  require(plane.points == samples, "Original samples changed");
  const Point n{plane.coefficients[0], plane.coefficients[1], plane.coefficients[2]};
  const auto& u = plane.axisU;
  const auto& v = plane.axisV;
  require(near(dot(u,u),1) && near(dot(v,v),1) && near(dot(n,n),1), "Non-unit frame");
  require(near(dot(u,v),0) && near(dot(u,n),0) && near(dot(v,n),0), "Non-orthogonal frame");
  const Point cross{u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]};
  require(near(dot(cross,n),1), "Left-handed frame");
  require(near(dot(plane.origin,n) + plane.coefficients[3],0), "Origin off plane");
  require(plane.width > 0 && plane.height > 0, "Empty rectangle");
  for (const auto& point : samples) {
    const double distance = dot(point,n) + plane.coefficients[3];
    Point delta{};
    for (int i = 0; i < 3; ++i) delta[i] = point[i] - distance*n[i] - plane.origin[i];
    require(std::abs(dot(delta,u)) <= plane.width/2 + 1e-7, "Projection outside width");
    require(std::abs(dot(delta,v)) <= plane.height/2 + 1e-7, "Projection outside height");
  }
}

void run()
{
  std::vector<Point> samples{{-100,-80,-20}, {100,-80,80}, {100,80,40}, {-100,80,-60}, {0,0,10}};
  const auto plane = BoundedPlane::fromPoints(samples);
  require(plane.has_value(), "Sample fit failed");
  auto data = std::make_shared<const BoundedPlane>(*plane);
  SceneModel scene;
  auto* object = scene.addBoundedPlane(QUrl("file:///sample.json"), "Sample", data);
  require(object != nullptr, "Scene insertion failed");
  auto* geometry = qobject_cast<PlaneGeometry*>(object->geometry());
  require(geometry && geometry->parent() == object, "Geometry is not scene-owned");
  require(&geometry->coefficients() == &data->coefficients, "Bounded coefficients duplicated");
  require(geometry->boundedPlane() == data, "Bounded data duplicated");
  verifyRectangle(*plane, samples);
  require(near(plane->width,222.28757209048453) && near(plane->height,189.1777875283397), "Incorrect sample dimensions");
  require(near(plane->origin[0],0) && near(plane->origin[1],0) && near(plane->origin[2],10), "Incorrect center");
  std::reverse(samples.begin(), samples.end());
  const auto reversed = BoundedPlane::fromPoints(samples);
  require(reversed.has_value(), "Reversed fit failed");
  verifyRectangle(*reversed, samples);
  require(near(plane->width,reversed->width) && near(plane->height,reversed->height), "Order-dependent bounds");
  for (int i = 0; i < 3; ++i) {
    require(near(plane->axisU[i],reversed->axisU[i]) && near(plane->origin[i],reversed->origin[i]), "Order-dependent frame");
  }
  samples[0][2] += 0.3;
  const auto noisy = BoundedPlane::fromPoints(samples);
  require(noisy.has_value(), "Noisy fit failed");
  verifyRectangle(*noisy, samples);

  const std::vector<Point> horizontal{{10,20,7}, {14,20,7}, {14,26,7}, {10,26,7}};
  const auto flat = BoundedPlane::fromPoints(horizontal);
  require(flat.has_value(), "Horizontal fit failed");
  verifyRectangle(*flat, horizontal);
  require(near(flat->width,4) && near(flat->height,6), "Incorrect flat dimensions");
  require(near(flat->origin[0],12) && near(flat->origin[1],23) && near(flat->origin[2],7), "Incorrect flat center");

  const std::vector<Point> steep{{-1,-2,-10}, {1,-2,10}, {1,2,10}, {-1,2,-10}};
  const auto fallback = BoundedPlane::fromPoints(steep);
  require(fallback.has_value(), "Steep fit failed");
  verifyRectangle(*fallback, steep);

  require(!BoundedPlane::fromPoints({}), "Accepted empty input");
  require(!BoundedPlane::fromPoints({{0,0,0},{1,1,1},{2,2,2}}), "Accepted collinear input");
  require(!BoundedPlane::fromPoints({{0,0,0},{0,1,0},{0,0,1}}), "Changed vertical fitter contract");
  samples[0][0] = std::numeric_limits<double>::quiet_NaN();
  require(!BoundedPlane::fromPoints(samples), "Accepted nonfinite input");

  QTemporaryFile file;
  require(file.open(), "Cannot create input fixture");
  file.write("[[0,0,1],[1,0,1],[0,1,1]]");
  file.flush();
  PlaneImporter importer;
  int failures = 0;
  int successes = 0;
  QObject::connect(&importer, &PlaneImporter::failed, [&failures]() { ++failures; });
  QObject::connect(&importer, &PlaneImporter::loaded, [&successes]() { ++successes; });
  auto* destination = new SceneModel;
  importer.load(QUrl::fromLocalFile(file.fileName()), destination);
  delete destination; // Before processing the queued worker completion.
  QElapsedTimer timer;
  timer.start();
  while (importer.busy() && timer.elapsed() < 10000) QCoreApplication::processEvents();
  require(!importer.busy() && failures == 1 && successes == 0, "Destroyed destination was not handled");
}

} // namespace

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  try {
    run();
    qInfo() << "Bounded plane regressions passed";
    return 0;
  } catch (const std::exception& error) {
    qCritical() << error.what();
    return 1;
  }
}
