#pragma once

#include "viewport/occcontroller.h"
#include "occt/occscene.h"
#include <QHash>

namespace RoboCrap3D {

class OccSceneAdapter final
{
public:
  // Borrows the controller's finished model loads, as the viewport borrows its document.
  OccSceneAdapter(OccScene& scene, const QHash<QString, std::shared_ptr<const CadLoadResult>>& models)
    : m_scene(scene), m_models(models) {}

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
    std::vector<OccScene::PartId> decorations; // Decorative object parts; diagnostic updates keep them.
    std::vector<OccScene::PartId> diagnostics; // Markers and normal lines only.
    bool visible = true;
    std::vector<OccScene::PartId> parts() const;
  };
  std::pair<Presentation, QString> createPresentation(const DrawingObject& object);
  QString removeParts(const std::vector<OccScene::PartId>& ids);

  OccScene& m_scene;
  const QHash<QString, std::shared_ptr<const CadLoadResult>>& m_models;
  QHash<quint32, Presentation> m_parts;
  std::vector<OccScene::PartId> m_axisParts;
};

} // namespace RoboCrap3D
