#pragma once

#include "3d/occcontroller.h"

#include <QJSEngine>
#include <QPointer>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

struct OccControllerQml
{
  Q_GADGET
  QML_FOREIGN(RoboCrap3D::OccController)
  QML_NAMED_ELEMENT(OccController)
  QML_SINGLETON

public:
  inline static RoboCrap3D::OccController* s_inst = nullptr;

  static RoboCrap3D::OccController* create(QQmlEngine*, QJSEngine* engine)
  {
    if (!s_inst || engine->thread() != s_inst->thread() || (s_engine && s_engine != engine))
    {
      qFatal("OccController must be used by one GUI-thread QML engine at a time");
    }

    s_engine = engine;
    QJSEngine::setObjectOwnership(s_inst, QJSEngine::CppOwnership);
    return s_inst;
  }

private:
  inline static QPointer<QJSEngine> s_engine;
};
