#pragma once

#include "3d/occt/occscene.h"

#include <QHash>
#include <QList>

class AIS_InteractiveObject;
class SceneModel;
class SceneObject;

namespace RoboCrap3D {

// Translates application objects to viewport-local parts owned by OccScene.
class OccSceneAdapter final
{
public:
  explicit OccSceneAdapter(OccScene& scene) : m_scene(scene) {}

  bool synchronize(const SceneModel* applicationScene);
  bool removeObject(quint32 objectId);
  bool synchronizeObject(const SceneObject* object, bool overlaysChanged = false);
  bool setDiagnosticOverlays(bool showPoints, bool showNormals, const SceneModel* applicationScene);
  void setSelectedObjects(const QList<quint32>& ids);
  quint32 objectIdFor(const Handle(AIS_InteractiveObject)& picked) const;

private:
  // Object, existing presentation and overlay kind identify the part to reconcile.
  std::optional<OccScene::PartId> synchronizeOverlay(
      const SceneObject* object, std::optional<OccScene::PartId> partId, bool normals);

  struct Presentation {
    OccScene::PartId surface;
    bool visible;
    std::optional<OccScene::PartId> points;
    std::optional<OccScene::PartId> normals;
  };

  OccScene& m_scene;
  QHash<quint32, Presentation> m_parts;
  bool m_showPoints = false;
  bool m_showNormals = false;
};

} // namespace RoboCrap3D
