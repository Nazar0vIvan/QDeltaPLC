#pragma once

#include "3d/occt/occscene.h"
#include "pathgeneration/chamfer/chamferpath.h"

#include <QHash>
#include <QList>

class AIS_InteractiveObject;
class SceneModel;
class SceneObject;
struct ChamferPathParameters;

namespace RoboCrap3D {

// Translates application objects to viewport-local parts owned by OccScene.
class OccSceneAdapter final
{
public:
  explicit OccSceneAdapter(OccScene& scene) : m_scene(scene) {}

  bool synchronize(const SceneModel* applicationScene);
  bool removeObject(quint32 objectId);
  bool synchronizeObject(const SceneObject* object, bool overlaysChanged = false);
  bool setDiagnosticOverlays(bool showPoints, bool showNormals, SceneModel* applicationScene);
  void setSelectedObjects(const QList<quint32>& ids);
  bool setMachiningPreview(const std::optional<ChamferPathParameters>& parameters);
  quint32 objectIdFor(const Handle(AIS_InteractiveObject)& picked) const;

private:
  // Object, existing presentation and overlay kind identify the part to reconcile.
  std::optional<OccScene::PartId> synchronizeOverlay(
      const SceneObject* object, std::optional<OccScene::PartId> partId, bool normals);

  struct Presentation {
    std::vector<OccScene::PartId> surfaces;
    bool visible;
    std::optional<OccScene::PartId> points;
    std::optional<OccScene::PartId> normals;
    bool showPoints = false;
    bool showNormals = false;
  };

  OccScene& m_scene;
  QHash<quint32, Presentation> m_parts;
  QList<quint32> m_selectedObjects;
  std::optional<OccScene::PartId> m_axisPreview;
  std::optional<ChamferPathParameters> m_previewParameters;
};

} // namespace RoboCrap3D
