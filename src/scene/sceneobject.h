#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include "scenegeometry.h"

class SceneModel;

class SceneObject final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(quint32 objectId READ objectId CONSTANT)
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(Kind kind READ kind CONSTANT)
  Q_PROPERTY(Classification classification READ classification CONSTANT)
  Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
  Q_PROPERTY(QUrl sourceUrl READ sourceUrl CONSTANT)
  Q_PROPERTY(QObject* geometry READ geometry CONSTANT)

public:
  enum Kind { Plane, Cylinder, Cone, Edge, ScanPath, MachiningPath, Circle };
  Q_ENUM(Kind)
  enum Classification { Unclassified, Rough, Precise };
  Q_ENUM(Classification)

  quint32 objectId() const { return m_objectId; }
  struct Description {
    QString name;
    Kind kind = Plane;
    Classification classification = Unclassified;
    QUrl sourceUrl;
  };

  const BoundedPlane* plane() const;
  const BoundedCylinder* cylinder() const;
  const ::Circle* circle() const;
  const EdgeGeometry* edge() const;
  QString name() const { return m_name; }
  Kind kind() const { return m_kind; }
  Classification classification() const { return m_classification; }
  bool visible() const { return m_visible; }
  QUrl sourceUrl() const { return m_sourceUrl; }
  QObject* geometry() const { return m_geometry; }

signals:
  void nameChanged();
  void visibleChanged();

private:
  friend class SceneModel;
  SceneObject(quint32 objectId, const Description& description, QObject* parent);
  void setPlaneGeometry(BoundedPlane plane);
  void setPlaneGeometry(const std::array<double, 4>& coefficients);
  void setCylinderGeometry(BoundedCylinder cylinder);
  void setCircleGeometry(::Circle circle);
  void setEdgeGeometry(EdgeGeometry edge, const std::array<quint32, 2>& sources);
  void setName(const QString& name);
  void setVisible(bool visible);

  const quint32 m_objectId;
  QString m_name;
  const Kind m_kind;
  const Classification m_classification;
  bool m_visible = true;
  const QUrl m_sourceUrl;
  QObject* m_geometry = nullptr;
};
