#pragma once

#include "scenemodel.h"

// Distinct type for the application-owned singleton; SceneModel also serves tests.
class ApplicationScene final : public SceneModel
{
  Q_OBJECT

public:
  explicit ApplicationScene(QObject* parent = nullptr) : SceneModel(parent) {}
};
