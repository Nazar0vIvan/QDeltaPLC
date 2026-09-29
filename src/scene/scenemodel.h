#pragma once

#include "sceneobject.h"
#include <QHash>
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
  Q_INVOKABLE SceneObject* findObject(quint32 objectId) const;
  Q_INVOKABLE void removeObjects(const QList<quint32>& ids);
  Q_INVOKABLE bool canIntersect(const QList<quint32>& selectedIds) const;
  Q_INVOKABLE QList<quint32> intersect(const QList<quint32>& selectedIds);
  SceneObject* addMachiningPath(SceneMachiningData data, const QString& name);
  // Source metadata, display name and geometry define a surface insertion.
	Q_INVOKABLE SceneObject* addPlane(const QUrl& sourceUrl, const QString& name, const QVariantList& coefficients);
	SceneObject* addBoundedPlane(const QUrl& sourceUrl, const QString& name, BoundedPlane plane);
	SceneObject* addBoundedCylinder(const QUrl& sourceUrl, const QString& name, BoundedCylinder cylinder);
	SceneObject* addCircle(const QUrl& sourceUrl, const QString& name, Circle circle);
  // Name, kind and classification describe metadata-only objects.
  // Metadata-only objects do not carry renderable geometry.
	Q_INVOKABLE SceneObject* addObject(const QString& name, SceneObject::Kind kind, SceneObject::Classification classification);
  Q_INVOKABLE bool renameObject(SceneObject* object, const QString& name);
  Q_INVOKABLE bool setObjectVisible(SceneObject* object, bool visible);

signals:
  void intersectionFailed(const QString& message);
  void objectsChanged();
  void objectAdded(SceneObject* object);
  void objectRemoved(quint32 objectId);
  void objectVisibilityChanged(SceneObject* object);

private:
  std::optional<std::array<const SceneObject*, 2>> intersectionPair(const QList<quint32>& ids) const;
  static qsizetype objectCount(QQmlListProperty<SceneObject>* list);
  static SceneObject* objectAt(QQmlListProperty<SceneObject>* list, qsizetype index);
  void appendObject(SceneObject* object);
  SceneObject* createObject(SceneObject::Description description);
  void onObjectVisibilityChanged();

  QList<SceneObject*> m_objects;
  QHash<quint32, SceneObject*> m_objectsById;
  quint32 m_nextId = 1; // Zero is never allocated, including after exhaustion.
};
