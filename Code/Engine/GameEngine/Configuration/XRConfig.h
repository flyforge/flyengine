#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class PL_GAMEENGINE_DLL plXRConfig : public plProfileConfigData
{
  PL_ADD_DYNAMIC_REFLECTION(plXRConfig, plProfileConfigData);

public:
  virtual void SaveRuntimeData(plChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(plChunkStreamReader& inout_stream) override;

  bool m_bEnableXR = false;
  plString m_sXRRenderPipeline;
};
