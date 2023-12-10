#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <GameEngine/GameEngineDLL.h>

class PLASMA_GAMEENGINE_DLL plRenderPipelineProfileConfig : public plProfileConfigData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderPipelineProfileConfig, plProfileConfigData);

public:
  virtual void SaveRuntimeData(plChunkStreamWriter& stream) const override;
  virtual void LoadRuntimeData(plChunkStreamReader& stream) override;

  plString m_sMainRenderPipeline;
  // plString m_sEditorRenderPipeline;
  // plString m_sDebugRenderPipeline;

  plMap<plString, plString> m_CameraPipelines;
};
