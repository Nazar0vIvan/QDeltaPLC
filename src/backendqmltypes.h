#pragma once

#include "network/common/deviceprofilemodel.h"
#include "network/devicehub.h"
#include "network/devicerunner.h"
#include "network/plc/plcmessagemanager.h"
#include "scene/scenesurfaceimporter.h"
#include "scene/scenemodel.h"
#include "scene/scenegeometry.h"
#include "scene/sceneendeffectors.h"
#include "scene/scenemachining.h"
#include "scene/scenemachiningpreparation.h"

#include <QJSEngine>
#include <QPointer>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

struct SceneMachiningQml
{
  Q_GADGET
  QML_FOREIGN(SceneMachining)
  QML_NAMED_ELEMENT(Machining)
  QML_SINGLETON
public:
  inline static SceneMachining* s_inst = nullptr;
  static SceneMachining* create(QQmlEngine*, QJSEngine* engine)
  {
    if (!s_inst || engine->thread() != s_inst->thread() || (s_engine && s_engine != engine))
      qFatal("Machining must be used by one GUI-thread QML engine at a time");
    s_engine = engine;
    QJSEngine::setObjectOwnership(s_inst, QJSEngine::CppOwnership);
    return s_inst;
  }
private:
  inline static QPointer<QJSEngine> s_engine;
};

struct SceneMachiningPreparationQml
{
  Q_GADGET
  QML_FOREIGN(SceneMachiningPreparation)
  QML_NAMED_ELEMENT(MachiningPreparation)
  QML_SINGLETON
public:
  inline static SceneMachiningPreparation* s_inst = nullptr;
  static SceneMachiningPreparation* create(QQmlEngine*, QJSEngine* engine)
  {
    if (!s_inst || engine->thread() != s_inst->thread() || (s_engine && s_engine != engine))
      qFatal("MachiningPreparation must be used by one GUI-thread QML engine at a time");
    s_engine = engine;
    QJSEngine::setObjectOwnership(s_inst, QJSEngine::CppOwnership);
    return s_inst;
  }
private:
  inline static QPointer<QJSEngine> s_engine;
};

struct SceneMachiningSettingsQml
{
  Q_GADGET
  QML_FOREIGN(SceneMachiningSettings)
  QML_VALUE_TYPE(machiningSettings)
};

struct SceneMachiningPathQml
{
  Q_GADGET
  QML_FOREIGN(SceneMachiningPath)
  QML_NAMED_ELEMENT(MachiningPath)
  QML_UNCREATABLE("Machining paths are owned by SceneModel")
};

struct SceneEndEffectorsQml
{
  Q_GADGET
  QML_FOREIGN(SceneEndEffectors)
  QML_NAMED_ELEMENT(EndEffectors)
  QML_SINGLETON

public:
  inline static SceneEndEffectors* s_inst = nullptr;

  static SceneEndEffectors* create(QQmlEngine*, QJSEngine* engine)
  {
    if (!s_inst || engine->thread() != s_inst->thread() || (s_engine && s_engine != engine))
      qFatal("EndEffectors must be used by one GUI-thread QML engine at a time");
    s_engine = engine;
    QJSEngine::setObjectOwnership(s_inst, QJSEngine::CppOwnership);
    return s_inst;
  }

private:
  inline static QPointer<QJSEngine> s_engine;
};

struct SceneObjectQml
{
  Q_GADGET
  QML_FOREIGN(SceneObject)
  QML_NAMED_ELEMENT(SceneObject)
  QML_UNCREATABLE("Scene objects are owned by SceneModel")
};

struct ScenePlaneGeometryQml
{
  Q_GADGET
  QML_FOREIGN(ScenePlaneGeometry)
  QML_NAMED_ELEMENT(PlaneGeometry)
  QML_UNCREATABLE("Plane geometry is owned by its scene object")
};

struct SceneCylinderGeometryQml
{
  Q_GADGET
  QML_FOREIGN(SceneCylinderGeometry)
  QML_NAMED_ELEMENT(CylinderGeometry)
  QML_UNCREATABLE("Cylinder geometry is owned by its scene object")
};

struct SceneCircleGeometryQml
{
  Q_GADGET
  QML_FOREIGN(SceneCircleGeometry)
  QML_NAMED_ELEMENT(CircleGeometry)
  QML_UNCREATABLE("Circle geometry is owned by its scene object")
};

struct SceneEdgeGeometryQml
{
  Q_GADGET
  QML_FOREIGN(SceneEdgeGeometry)
  QML_NAMED_ELEMENT(EdgeGeometry)
  QML_UNCREATABLE("Edge geometry is owned by its scene object")
};

struct SceneModelQml
{
  Q_GADGET
  QML_FOREIGN(SceneModel)
  QML_NAMED_ELEMENT(SceneModel)
};

struct SceneSingletonQml
{
  Q_GADGET
  QML_FOREIGN(SceneModel)
  QML_NAMED_ELEMENT(Scene)
  QML_SINGLETON

public:
  inline static SceneModel* s_inst = nullptr;

  static SceneModel* create(QQmlEngine*, QJSEngine* engine)
  {
    Q_ASSERT(s_inst);
    Q_ASSERT(engine->thread() == s_inst->thread());
    if (s_engine)
      Q_ASSERT(engine == s_engine);
    else
      s_engine = engine;
    QJSEngine::setObjectOwnership(s_inst, QJSEngine::CppOwnership);
    return s_inst;
  }

private:
  inline static QJSEngine* s_engine = nullptr;
};

struct SceneSurfaceImporterQml
{
  Q_GADGET
  QML_FOREIGN(SceneSurfaceImporter)
  QML_NAMED_ELEMENT(PlaneImporter)
};

struct DeviceRunnerQml
{
  Q_GADGET
  QML_FOREIGN(DeviceRunner)
  QML_NAMED_ELEMENT(DeviceRunner)
  QML_UNCREATABLE("Get DeviceRunner from Hub.device()")
};

struct DeviceProfileModelQml
{
  Q_GADGET
  QML_FOREIGN(DeviceProfileModel)
  QML_NAMED_ELEMENT(DeviceProfileModel)
};

struct PlcMessageQml
{
  Q_GADGET
  QML_FOREIGN(PlcMessageManager)
  QML_NAMED_ELEMENT(PlcMessage)
  QML_UNCREATABLE("PlcMessage is not creatable from QML")
};

struct DeviceHubQml
{
  Q_GADGET
  QML_FOREIGN(DeviceHub)
  QML_NAMED_ELEMENT(Hub)
  QML_SINGLETON

public:
  inline static DeviceHub* s_inst = nullptr;

  static DeviceHub* create(QQmlEngine*, QJSEngine* engine)
  {
    Q_ASSERT(s_inst);
    Q_ASSERT(engine->thread() == s_inst->thread());

    if (s_engine)
      Q_ASSERT(engine == s_engine);
    else
      s_engine = engine;

    QJSEngine::setObjectOwnership(s_inst, QJSEngine::CppOwnership);
    return s_inst;
  }

private:
  inline static QJSEngine* s_engine = nullptr;
};
