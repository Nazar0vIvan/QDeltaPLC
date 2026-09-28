#pragma once

#include "geometry/boundedplane.h"
#include "geometry/boundedcylinder.h"
#include "geometry/circle.h"
#include "geometry/surfaceintersection.h"
#include <QList>
#include <QObject>
#include <QVariantList>
#include <optional>
#include <utility>

// Read-only presentation of a fitted plane; fitting stays in geometry/plane.cpp.
class ScenePlaneGeometry final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double normalX READ normalX CONSTANT)
  Q_PROPERTY(double normalY READ normalY CONSTANT)
  Q_PROPERTY(double normalZ READ normalZ CONSTANT)
  Q_PROPERTY(double offset READ offset CONSTANT)
  Q_PROPERTY(bool hasBounds READ hasBounds CONSTANT)
  Q_PROPERTY(double originX READ originX CONSTANT)
  Q_PROPERTY(double originY READ originY CONSTANT)
  Q_PROPERTY(double originZ READ originZ CONSTANT)
  Q_PROPERTY(QVariantList axisU READ axisU CONSTANT)
  Q_PROPERTY(QVariantList axisV READ axisV CONSTANT)
  Q_PROPERTY(double width READ width CONSTANT)
  Q_PROPERTY(double height READ height CONSTANT)
  Q_PROPERTY(qsizetype pointCount READ pointCount CONSTANT)

public:
  explicit ScenePlaneGeometry(const std::array<double, 4>& coefficients, QObject* parent)
    : QObject(parent), m_coefficients(coefficients) {}

  explicit ScenePlaneGeometry(BoundedPlane plane, QObject* parent)
    : QObject(parent), m_plane(std::move(plane)) {}

  bool hasBounds() const { return m_plane.has_value(); }
  double originX() const { return m_plane ? m_plane->origin()[0] : 0.0; }
  double originY() const { return m_plane ? m_plane->origin()[1] : 0.0; }
  double originZ() const { return m_plane ? m_plane->origin()[2] : 0.0; }
  QVariantList axisU() const {
    return m_plane ? QVariantList{m_plane->axisU()[0], m_plane->axisU()[1], m_plane->axisU()[2]} : QVariantList{};
  }
  QVariantList axisV() const {
    return m_plane ? QVariantList{m_plane->axisV()[0], m_plane->axisV()[1], m_plane->axisV()[2]} : QVariantList{};
  }
  double width() const { return m_plane ? m_plane->width() : 0.0; }
  double height() const { return m_plane ? m_plane->height() : 0.0; }
  qsizetype pointCount() const { return m_plane ? static_cast<qsizetype>(m_plane->points().size()) : 0; }
  const BoundedPlane* boundedPlane() const { return m_plane ? &*m_plane : nullptr; }

  double normalX() const { return coefficients()[0]; }
  double normalY() const { return coefficients()[1]; }
  double normalZ() const { return coefficients()[2]; }
  double offset() const { return coefficients()[3]; }
  const std::array<double, 4>& coefficients() const {
    return m_plane ? m_plane->coefficients() : *m_coefficients;
  }

private:
  // Unit normal and offset: nx*x + ny*y + nz*z + offset = 0.
  // Present only for the coefficient-only API; fitted data is moved into m_plane.
  const std::optional<std::array<double, 4>> m_coefficients;
  const std::optional<BoundedPlane> m_plane;
};

// Read-only QML view of the scene-owned numerical cylinder.
class SceneCylinderGeometry final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double originX READ originX CONSTANT)
  Q_PROPERTY(double originY READ originY CONSTANT)
  Q_PROPERTY(double originZ READ originZ CONSTANT)
  Q_PROPERTY(double axisX READ axisX CONSTANT)
  Q_PROPERTY(double axisY READ axisY CONSTANT)
  Q_PROPERTY(double axisZ READ axisZ CONSTANT)
  Q_PROPERTY(double radius READ radius CONSTANT)
  Q_PROPERTY(double length READ length CONSTANT)
  Q_PROPERTY(qsizetype pointCount READ pointCount CONSTANT)

public:
  explicit SceneCylinderGeometry(BoundedCylinder cylinder, QObject* parent)
    : QObject(parent), m_cylinder(std::move(cylinder)) {}

  double originX() const { return m_cylinder.origin()[0]; }
  double originY() const { return m_cylinder.origin()[1]; }
  double originZ() const { return m_cylinder.origin()[2]; }
  double axisX() const { return m_cylinder.axis()[0]; }
  double axisY() const { return m_cylinder.axis()[1]; }
  double axisZ() const { return m_cylinder.axis()[2]; }
  double radius() const { return m_cylinder.radius(); }
  double length() const { return m_cylinder.length(); }
  qsizetype pointCount() const { return static_cast<qsizetype>(m_cylinder.points().size()); }
  const BoundedCylinder& boundedCylinder() const { return m_cylinder; }

private:
  const BoundedCylinder m_cylinder;
};

// Read-only QML view of the scene-owned numerical circle.
class SceneCircleGeometry final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double centerX READ centerX CONSTANT)
  Q_PROPERTY(double centerY READ centerY CONSTANT)
  Q_PROPERTY(double centerZ READ centerZ CONSTANT)
  Q_PROPERTY(double normalX READ normalX CONSTANT)
  Q_PROPERTY(double normalY READ normalY CONSTANT)
  Q_PROPERTY(double normalZ READ normalZ CONSTANT)
  Q_PROPERTY(double radius READ radius CONSTANT)
  Q_PROPERTY(double rmsResidual READ rmsResidual CONSTANT)
  Q_PROPERTY(qsizetype pointCount READ pointCount CONSTANT)

public:
  explicit SceneCircleGeometry(Circle circle, QObject* parent)
    : QObject(parent), m_circle(std::move(circle)) {}

  double centerX() const { return m_circle.center().x(); }
  double centerY() const { return m_circle.center().y(); }
  double centerZ() const { return m_circle.center().z(); }
  double normalX() const { return m_circle.normal().x(); }
  double normalY() const { return m_circle.normal().y(); }
  double normalZ() const { return m_circle.normal().z(); }
  double radius() const { return m_circle.radius(); }
  double rmsResidual() const { return m_circle.rmsResidual(); }
  qsizetype pointCount() const { return m_circle.points().size(); }
  const Circle& circle() const { return m_circle; }

private:
  const Circle m_circle;
};

class SceneEdgeGeometry final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(CurveKind curveKind READ curveKind CONSTANT)
  Q_PROPERTY(QList<quint32> sourceIds READ sourceIds CONSTANT)

public:
  enum CurveKind { Ellipse = 1, Circle = 2 };
  Q_ENUM(CurveKind)

  // Numerical geometry, source identity and QObject ownership define an edge.
  SceneEdgeGeometry(EdgeGeometry edge, const std::array<quint32, 2>& sources, QObject* parent)
    : QObject(parent), m_edge(std::move(edge)), m_sources(sources) {}

  CurveKind curveKind() const {
    return m_edge.ellipse().isCircle() ? Circle : Ellipse;
  }
  QList<quint32> sourceIds() const { return {m_sources[0], m_sources[1]}; }
  const EdgeGeometry& edge() const { return m_edge; }

private:
  const EdgeGeometry m_edge;
  const std::array<quint32, 2> m_sources;
};
