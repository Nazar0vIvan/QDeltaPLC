#include "sceneobject.h"

SceneObject::SceneObject(const QString& objectId, const QString& name, Kind kind,
                         Classification classification, const QUrl& sourceUrl, QObject* parent)
  : QObject(parent), m_objectId(objectId), m_name(name), m_kind(kind),
    m_classification(classification), m_sourceUrl(sourceUrl)
{}

void SceneObject::setName(const QString& name)
{
  if (m_name == name) return;
  m_name = name;
  emit nameChanged();
}

void SceneObject::setVisible(bool visible)
{
  if (m_visible == visible) return;
  m_visible = visible;
  emit visibleChanged();
}
