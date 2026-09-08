#pragma once

#include "network/common/deviceprofilemodel.h"
#include "network/devicehub.h"
#include "network/devicerunner.h"
#include "network/plc/plcmessagemanager.h"

#include <QJSEngine>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

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
