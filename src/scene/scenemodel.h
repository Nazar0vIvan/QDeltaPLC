#pragma once

#include "sceneobject.h"
#include "planegeometry.h"
#include "geometry/boundedcylinder.h"

#include <QList>
#include <QQmlListProperty>
#include <QVariantList>

class SceneModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QQmlListProperty<SceneObject> objects READ objects NOTIFY objectsChanged)

public:
  explicit SceneModel(QObject* parent = nullptr);

  QQmlListProperty<SceneObject> objects();
  const QList<SceneObject*>& objectList() const { return m_objects; }
  Q_INVOKABLE SceneObject* findObject(const QString& objectId) const;
  Q_INVOKABLE SceneObject* addPlane(const QUrl& sourceUrl, const QString& name,
                                   const QVariantList& coefficients);
  SceneObject* addBoundedPlane(const QUrl& sourceUrl, const QString& name,
                               std::shared_ptr<const BoundedPlane> plane);
  SceneObject* addBoundedCylinder(const QUrl& sourceUrl, const QString& name,
                                  std::shared_ptr<const BoundedCylinder> cylinder);
  // Metadata-only objects preserve the current sample scene until producers exist.
  Q_INVOKABLE SceneObject* addObject(const QString& objectId, const QString& name,
                                    SceneObject::Kind kind, SceneObject::Classification classification);
  Q_INVOKABLE bool renameObject(SceneObject* object, const QString& name);
  Q_INVOKABLE bool setObjectVisible(SceneObject* object, bool visible);

signals:
  void objectsChanged();

private:
  static qsizetype objectCount(QQmlListProperty<SceneObject>* list);
  static SceneObject* objectAt(QQmlListProperty<SceneObject>* list, qsizetype index);
  void appendObject(SceneObject* object);
  SceneObject* createPlane(const QUrl& sourceUrl, const QString& name,
                           const std::array<double, 4>& coefficients,
                           std::shared_ptr<const BoundedPlane> bounded = {});

  QList<SceneObject*> m_objects;
  quint64 m_nextPlaneId = 1;
  quint64 m_nextCylinderId = 1;
};
