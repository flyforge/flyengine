#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class PL_GAMEENGINE_DLL plRenderPipelineProfileConfig : public plProfileConfigData
{
  PL_ADD_DYNAMIC_REFLECTION(plRenderPipelineProfileConfig, plProfileConfigData);

public:
  virtual void SaveRuntimeData(plChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(plChunkStreamReader& inout_stream) override;

  plString m_sMainRenderPipeline;
  // plString m_sEditorRenderPipeline;
  // plString m_sDebugRenderPipeline;

  plMap<plString, plString> m_CameraPipelines;
};
