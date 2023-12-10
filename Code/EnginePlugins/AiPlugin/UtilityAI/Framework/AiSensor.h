#pragma once

#include <GameEngine/GameEngineDLL.h>

class plGameObject;

class PLASMA_AIPLUGIN_DLL plAiSensor
{
public:
  plAiSensor() = default;
  virtual ~plAiSensor() = default;

  virtual void UpdateSensor(plGameObject& owner) = 0;
};
