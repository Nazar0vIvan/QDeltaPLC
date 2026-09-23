#pragma once

#include <QObject>
#include <QUrl>
#include <QPointer>
#include <memory>
#include "scene/scenemodel.h"

class QThread;

class PlaneImporter : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
  explicit PlaneImporter(QObject* parent = nullptr);
  ~PlaneImporter() override;

  bool busy() const { return m_worker != nullptr; }
  Q_INVOKABLE void load(const QUrl& sourceUrl, SceneModel* scene);
  Q_INVOKABLE void loadCylinder(const QUrl& sourceUrl, SceneModel* scene);

signals:
  void busyChanged();
  void loaded(SceneObject* object);
  void failed(const QString& message);

private:
  enum class Surface { Plane, Cylinder };
  struct ImportResult;
  static void fitSurface(const QString& path, std::shared_ptr<ImportResult> result, Surface surface);
  void finishImport(std::shared_ptr<ImportResult> result, const QUrl& sourceUrl,
                    const QString& path, QPointer<SceneModel> destination, Surface surface);
  void startImport(const QUrl& sourceUrl, SceneModel* scene, Surface surface);
  QThread* m_worker = nullptr;
};
