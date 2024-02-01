#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <GameEngine/GameEngineDLL.h>

class PL_GAMEENGINE_DLL plSimpleWindWorldModule : public plWindWorldModuleInterface
{
  PL_DECLARE_WORLD_MODULE();
  PL_ADD_DYNAMIC_REFLECTION(plSimpleWindWorldModule, plWindWorldModuleInterface);

public:
  plSimpleWindWorldModule(plWorld* pWorld);
  ~plSimpleWindWorldModule();

  virtual plVec3 GetWindAt(const plVec3& vPosition) const override;

  void SetFallbackWind(const plVec3& vWind);

private:
  plVec3 m_vFallbackWind;
};
