#pragma once

#include "sceneobject.h"
#include <QHash>
#include <QList>
#include <QQmlListProperty>
#include <QPointer>

namespace RoboCrap3D { class OccController; }

class SceneModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QQmlListProperty<SceneObject> objects READ objects NOTIFY objectsChanged)

public:
  explicit SceneModel(QObject* parent = nullptr);

  void setViewport(RoboCrap3D::OccController* viewport);
  QQmlListProperty<SceneObject> objects();
  const QList<SceneObject*>& objectList() const { return m_objects; }
  Q_INVOKABLE SceneObject* findObject(quint32 objectId) const;
  Q_INVOKABLE void removeObjects(const QList<quint32>& ids);
  Q_INVOKABLE bool canIntersect(const QList<quint32>& selectedIds) const;
  Q_INVOKABLE QList<quint32> intersect(const QList<quint32>& selectedIds);
  SceneObject* addMachiningPath(SceneMachiningData data, const QString& name);
	SceneObject* addBoundedPlane(const QUrl& sourceUrl, const QString& name, BoundedPlane plane);
	SceneObject* addBoundedCylinder(const QUrl& sourceUrl, const QString& name, BoundedCylinder cylinder);
  // A calibration sphere or calibration path; the shape decides the kind.
  SceneObject* addCalibrationObject(SceneObject::Shape shape, const QString& name);
  Q_INVOKABLE bool renameObject(SceneObject* object, const QString& name);
  Q_INVOKABLE bool setObjectVisible(SceneObject* object, bool visible);
  Q_INVOKABLE bool setObjectDiagnosticOverlays(SceneObject* object, bool showPoints, bool showNormals);
  // Reinstalls the sphere drawing with the requested spheres; keeps the old state on failure.
  Q_INVOKABLE bool setSphereDisplay(SceneObject* object, bool ballCenter, bool compensated);

signals:
  void intersectionFailed(const QString& message);
  void objectsChanged();
  void objectAdded(SceneObject* object);
  void objectRemoved(quint32 objectId);
  void objectVisibilityChanged(SceneObject* object);
  void objectDiagnosticOverlaysChanged(SceneObject* object);

private:
  std::optional<std::array<const SceneObject*, 2>> intersectionPair(const QList<quint32>& ids) const;
  static qsizetype objectCount(QQmlListProperty<SceneObject>* list);
  static SceneObject* objectAt(QQmlListProperty<SceneObject>* list, qsizetype index);
  SceneObject* appendObject(SceneObject* object);
  SceneObject* createObject(SceneObject::Description description);
  void onObjectVisibilityChanged();
  void onObjectDiagnosticOverlaysChanged();

  QPointer<RoboCrap3D::OccController> m_viewport;
  QList<SceneObject*> m_objects;
  QHash<quint32, SceneObject*> m_objectsById;
  quint32 m_nextId = 1; // Zero is never allocated, including after exhaustion.
  quint64 m_nextEdgeNumber = 1;
  quint64 m_nextMachiningPathNumber = 1;
};
