#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class PLASMA_GAMEENGINE_DLL plXRConfig : public plProfileConfigData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plXRConfig, plProfileConfigData);

public:
  virtual void SaveRuntimeData(plChunkStreamWriter& stream) const override;
  virtual void LoadRuntimeData(plChunkStreamReader& stream) override;

  bool m_bEnableXR = false;
  plString m_sXRRenderPipeline;
};
