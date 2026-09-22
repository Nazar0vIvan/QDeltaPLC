#pragma once

#include <QObject>
#include <QVariantList>
#include "geometry/boundedplane.h"

#include <array>
#include <memory>
#include <optional>
#include <utility>

// Read-only presentation of a fitted plane; fitting stays in geometry/plane.cpp.
class PlaneGeometry final : public QObject
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
  explicit PlaneGeometry(const std::array<double, 4>& coefficients, QObject* parent)
    : QObject(parent), m_coefficients(coefficients) {}

  explicit PlaneGeometry(std::shared_ptr<const BoundedPlane> plane, QObject* parent)
    : QObject(parent), m_plane(std::move(plane)) { Q_ASSERT(m_plane); }

  bool hasBounds() const { return m_plane != nullptr; }
  double originX() const { return m_plane ? m_plane->origin[0] : 0.0; }
  double originY() const { return m_plane ? m_plane->origin[1] : 0.0; }
  double originZ() const { return m_plane ? m_plane->origin[2] : 0.0; }
  QVariantList axisU() const {
    return m_plane ? QVariantList{m_plane->axisU[0], m_plane->axisU[1], m_plane->axisU[2]} : QVariantList{};
  }
  QVariantList axisV() const {
    return m_plane ? QVariantList{m_plane->axisV[0], m_plane->axisV[1], m_plane->axisV[2]} : QVariantList{};
  }
  double width() const { return m_plane ? m_plane->width : 0.0; }
  double height() const { return m_plane ? m_plane->height : 0.0; }
  qsizetype pointCount() const { return m_plane ? static_cast<qsizetype>(m_plane->points.size()) : 0; }
  const std::shared_ptr<const BoundedPlane>& boundedPlane() const { return m_plane; }

  double normalX() const { return coefficients()[0]; }
  double normalY() const { return coefficients()[1]; }
  double normalZ() const { return coefficients()[2]; }
  double offset() const { return coefficients()[3]; }
  const std::array<double, 4>& coefficients() const {
    return m_plane ? m_plane->coefficients : *m_coefficients;
  }

private:
  // Unit normal and offset: nx*x + ny*y + nz*z + offset = 0.
  // Present only for the coefficient-only API; bounded data is never copied here.
  const std::optional<std::array<double, 4>> m_coefficients;
  const std::shared_ptr<const BoundedPlane> m_plane;
};
