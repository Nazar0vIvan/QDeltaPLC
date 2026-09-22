#pragma once

#include "geometry/boundedcylinder.h"

#include <QObject>

#include <memory>
#include <utility>

// Read-only QML view of the scene-owned numerical cylinder.
class CylinderGeometry final : public QObject
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
  explicit CylinderGeometry(std::shared_ptr<const BoundedCylinder> cylinder, QObject* parent)
    : QObject(parent), m_cylinder(std::move(cylinder)) { Q_ASSERT(m_cylinder); }

  double originX() const { return m_cylinder->origin[0]; }
  double originY() const { return m_cylinder->origin[1]; }
  double originZ() const { return m_cylinder->origin[2]; }
  double axisX() const { return m_cylinder->axis[0]; }
  double axisY() const { return m_cylinder->axis[1]; }
  double axisZ() const { return m_cylinder->axis[2]; }
  double radius() const { return m_cylinder->radius; }
  double length() const { return m_cylinder->length; }
  qsizetype pointCount() const { return static_cast<qsizetype>(m_cylinder->points.size()); }
  const std::shared_ptr<const BoundedCylinder>& boundedCylinder() const { return m_cylinder; }

private:
  const std::shared_ptr<const BoundedCylinder> m_cylinder;
};
