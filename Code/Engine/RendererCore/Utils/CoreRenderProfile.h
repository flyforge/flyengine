#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/RendererCoreDLL.h>

class PL_RENDERERCORE_DLL plCoreRenderProfileConfig : public plProfileConfigData
{
  PL_ADD_DYNAMIC_REFLECTION(plCoreRenderProfileConfig, plProfileConfigData);

public:
  virtual void SaveRuntimeData(plChunkStreamWriter& inout_stream) const override;
  virtual void LoadRuntimeData(plChunkStreamReader& inout_stream) override;

  plUInt32 m_uiShadowAtlasTextureSize = 4096;
  plUInt32 m_uiMaxShadowMapSize = 1024;
  plUInt32 m_uiMinShadowMapSize = 64;
};

PL_RENDERERCORE_DLL extern plCVarInt cvar_RenderingShadowsAtlasSize;
PL_RENDERERCORE_DLL extern plCVarInt cvar_RenderingShadowsMaxShadowMapSize;
PL_RENDERERCORE_DLL extern plCVarInt cvar_RenderingShadowsMinShadowMapSize;
