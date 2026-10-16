#pragma once

#include "viewport/occcontroller.h"
#include "occt/occscene.h"
#include <QHash>

namespace RoboCrap3D {

class OccSceneAdapter final
{
public:
  explicit OccSceneAdapter(OccScene& scene) : m_scene(scene) {}

  ViewportOperationResult insertObject(const DrawingObject& object);
  bool removeObject(quint32 id);
  ViewportOperationResult setObjectVisible(quint32 id, bool visible);
  ViewportOperationResult setObjectDiagnostics(quint32 id, const DrawingDiagnostics& diagnostics);
  ViewportOperationResult setDraftAxis(const std::optional<DrawingAxis>& axis);
  void setSelectedObjects(const QList<quint32>& ids);
  quint32 objectIdFor(const Handle(AIS_InteractiveObject)& picked) const;

private:
  struct Presentation {
    std::vector<OccScene::PartId> surfaces;
    std::vector<OccScene::PartId> diagnostics;
    bool visible = true;
  };
  std::pair<Presentation, QString> createPresentation(const DrawingObject& object);
  QString removeParts(const std::vector<OccScene::PartId>& surfaces,
                      const std::vector<OccScene::PartId>& diagnostics = {});

  OccScene& m_scene;
  QHash<quint32, Presentation> m_parts;
  std::vector<OccScene::PartId> m_axisParts;
};

} // namespace RoboCrap3D
