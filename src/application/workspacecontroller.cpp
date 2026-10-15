#include "workspacecontroller.h"

#include "scene/scenemodel.h"
#include "scene/scenemachining.h"
#include "3d/robot/robotpreviewstate.h"

#include <utility>

WorkspaceController::WorkspaceController(SceneModel* scene, SceneMachining* machining,
                                         RoboCrap3D::RobotController* robot)
  : m_scene(scene), m_machining(machining), m_robot(robot)
{
  Q_ASSERT(scene && machining && robot && !s_instance);
  Q_ASSERT(scene->thread() == thread() && machining->thread() == thread() && robot->thread() == thread());
  s_instance = this;
  connect(scene, &SceneModel::objectsChanged, this, &WorkspaceController::pruneSelection);
}

WorkspaceController* WorkspaceController::create(QQmlEngine*, QJSEngine* engine)
{
  if (!s_instance || engine->thread() != s_instance->thread() || (s_engine && s_engine != engine))
    qFatal("WorkspaceController must be used by one GUI-thread QML engine at a time");
  s_engine = engine;
  QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
  return s_instance;
}

void WorkspaceController::setMode(Mode mode)
{
  if ((mode != Measuring && mode != Machining) || mode == m_mode) return;
  m_mode = mode;
  synchronizeMode();
}

void WorkspaceController::synchronizeMode()
{
  if (m_mode != Machining) m_machining->stop();
  // QML cancels the old editor before tool/input settings change.
  emit modeChanged();
  m_robot->setActiveTool(m_mode == Machining ? SceneEndEffectors::Spindle : SceneEndEffectors::Measuring);
  synchronizeMachiningInput();
}

void WorkspaceController::setSubmode(Submode submode)
{
  if ((submode != Rough && submode != Precise) || submode == m_submode) return;
  m_submode = submode;
  emit submodeChanged();
}

SceneObject* WorkspaceController::selectedObject() const
{
  return m_selectedIds.size() == 1 ? m_scene->findObject(m_selectedIds.front()) : nullptr;
}

bool WorkspaceController::intersectionAvailable() const
{
  return m_scene->canIntersect(m_selectedIds);
}

void WorkspaceController::selectObject(quint32 id, bool additive)
{
  if (id && !m_scene->findObject(id)) return;
  auto ids = additive ? m_selectedIds : QList<quint32>{};
  if (!id) ids.clear();
  else if (!ids.removeOne(id)) ids.append(id);
  setSelection(std::move(ids));
}

void WorkspaceController::setSelection(QList<quint32> ids)
{
  if (ids == m_selectedIds) return;
  m_selectedIds = std::move(ids);
  // Derived selection notification lets QML cancel before the new input applies its settings.
  emit selectionDetailsChanged();
  synchronizeMachiningInput();
  emit selectionChanged(m_selectedIds);
}

void WorkspaceController::pruneSelection()
{
  auto ids = m_selectedIds;
  for (quint32 id : m_selectedIds)
    if (!m_scene->findObject(id)) ids.removeOne(id);
  setSelection(std::move(ids));
}

void WorkspaceController::synchronizeMachiningInput()
{
  const auto* object = selectedObject();
  m_machining->setInputId(m_mode == Machining && object
      && (object->kind() == SceneObject::Edge || object->kind() == SceneObject::MachiningPath)
      ? object->objectId() : 0);
}

void WorkspaceController::deleteSelected()
{
  const auto ids = m_selectedIds;
  setSelection({});
  m_scene->removeObjects(ids);
}

void WorkspaceController::intersectSelected()
{
  auto ids = m_scene->intersect(m_selectedIds);
  if (!ids.isEmpty()) setSelection(std::move(ids));
}

void WorkspaceController::reset()
{
  m_mode = Measuring;
  setSubmode(Rough);
  synchronizeMode();
  setSelection({});
}
