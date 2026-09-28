#pragma once

#include <QObject>
#include <QUrl>

class SceneModel;
class SceneObject;

// Synchronous GUI-thread import into the Scene Abstraction Layer.
class SceneSurfaceImporter : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
  explicit SceneSurfaceImporter(QObject* parent = nullptr);
  bool busy() const { return m_busy; }
  Q_INVOKABLE void load(const QUrl& sourceUrl, SceneModel* scene);
  Q_INVOKABLE void loadCylinder(const QUrl& sourceUrl, SceneModel* scene);
  Q_INVOKABLE void loadCircle(const QUrl& sourceUrl, SceneModel* scene);

signals:
  void busyChanged();
  void loaded(SceneObject* object);
  void failed(const QString& message);

private:
  enum class Surface { Plane, Cylinder, Circle };
  // Source, destination and requested surface define one import operation.
  void importSurface(const QUrl& sourceUrl, SceneModel* scene, Surface surface);
  bool m_busy = false;
};
