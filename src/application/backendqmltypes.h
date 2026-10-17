#pragma once

#include "devices/fts/rdtmessage.h"
#include "devices/deviceprofilemodel.h"
#include "devices/devicehub.h"
#include "devices/devicerunner.h"
#include "devices/plc/plcmessagemanager.h"
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

// Hands a main()-owned singleton to its one GUI-thread QML engine. CppOwnership keeps the
// engine from deleting it (Qt's documented QML_FOREIGN + create() instance pattern).
inline QObject* claimQmlSingleton(QObject* instance, QJSEngine* engine)
{
  static QPointer<QJSEngine> claimedEngine;
  if (!instance || engine->thread() != instance->thread() || (claimedEngine && claimedEngine != engine))
    qFatal("%s must be used by one GUI-thread QML engine at a time",
           instance ? instance->metaObject()->className() : "A QML singleton");
  claimedEngine = engine;
  QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
  return instance;
}

struct SceneMachiningQml
{
  Q_GADGET
  QML_FOREIGN(SceneMachining)
  QML_NAMED_ELEMENT(Machining)
  QML_SINGLETON
public:
  inline static SceneMachining* s_inst = nullptr;
  static SceneMachining* create(QQmlEngine*, QJSEngine* engine)
  { return static_cast<SceneMachining*>(claimQmlSingleton(s_inst, engine)); }
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
  { return static_cast<SceneMachiningPreparation*>(claimQmlSingleton(s_inst, engine)); }
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
  { return static_cast<SceneEndEffectors*>(claimQmlSingleton(s_inst, engine)); }
};

struct SceneObjectQml
{
  Q_GADGET
  QML_FOREIGN(SceneObject)
  QML_NAMED_ELEMENT(SceneObject)
  QML_UNCREATABLE("Scene objects are owned by SceneModel")
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
  { return static_cast<SceneModel*>(claimQmlSingleton(s_inst, engine)); }
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
  { return static_cast<DeviceHub*>(claimQmlSingleton(s_inst, engine)); }
};

struct RDTResponseQml
{
  Q_GADGET
  QML_FOREIGN(RDTResponse)
  QML_VALUE_TYPE(rdtResponse)
};
