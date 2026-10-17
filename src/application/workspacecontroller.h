#pragma once

#include "scene/sceneobject.h"

#include <QObject>
#include <QList>
#include <QPointer>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

class SceneModel;
class SceneMachining;
class SceneEndEffectors;

class WorkspaceController final : public QObject
{
  Q_OBJECT
  QML_NAMED_ELEMENT(WorkspaceController)
  QML_SINGLETON
  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(Submode submode READ submode WRITE setSubmode NOTIFY submodeChanged)
  Q_PROPERTY(QList<quint32> selectedObjectIds READ selectedObjectIds NOTIFY selectionChanged)
  Q_PROPERTY(SceneObject* selectedObject READ selectedObject NOTIFY selectionDetailsChanged)
  Q_PROPERTY(bool intersectionAvailable READ intersectionAvailable NOTIFY selectionDetailsChanged)
public:
  enum Mode { Measuring, Machining };
  Q_ENUM(Mode)
  enum Submode { Rough, Precise };
  Q_ENUM(Submode)

  // Document operations, machining lifecycle and the active tool are distinct collaborators.
  WorkspaceController(SceneModel* scene, SceneMachining* machining, SceneEndEffectors* effectors);
  static WorkspaceController* create(QQmlEngine*, QJSEngine* engine);

  Mode mode() const { return m_mode; }
  void setMode(Mode mode);
  Submode submode() const { return m_submode; }
  void setSubmode(Submode submode);
  const QList<quint32>& selectedObjectIds() const { return m_selectedIds; }
  SceneObject* selectedObject() const;
  bool intersectionAvailable() const;
  Q_INVOKABLE void selectObject(quint32 id, bool additive);
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE void intersectSelected();
  Q_INVOKABLE void reset();

signals:
  void modeChanged();
  void submodeChanged();
  void selectionChanged(const QList<quint32>& ids);
  void selectionDetailsChanged();

private:
  void setSelection(QList<quint32> ids);
  void pruneSelection();
  void synchronizeMode();
  void synchronizeMachiningInput();

  SceneModel* m_scene;
  SceneMachining* m_machining;
  SceneEndEffectors* m_effectors;
  Mode m_mode = Measuring;
  Submode m_submode = Rough;
  QList<quint32> m_selectedIds;
  inline static QPointer<WorkspaceController> s_instance;
};
