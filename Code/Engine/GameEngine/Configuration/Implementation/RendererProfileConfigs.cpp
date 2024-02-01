#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Configuration/RendererProfileConfigs.h>

#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineProfileConfig, 1, plRTTIDefaultAllocator<plRenderPipelineProfileConfig>)
{
  PL_BEGIN_PROPERTIES
  {
    // MainRenderPipeline.plRenderPipelineAsset
    PL_MEMBER_PROPERTY("MainRenderPipeline", m_sMainRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"))),
    // EditorRenderPipeline.plRenderPipelineAsset
    //PL_MEMBER_PROPERTY("EditorRenderPipeline", m_sEditorRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"))),
    // DebugRenderPipeline.plRenderPipelineAsset
    //PL_MEMBER_PROPERTY("DebugRenderPipeline", m_sDebugRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"))),

    PL_MAP_MEMBER_PROPERTY("CameraPipelines", m_CameraPipelines)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plRenderPipelineProfileConfig::SaveRuntimeData(plChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("plRenderPipelineProfileConfig", 2);

  inout_stream << m_sMainRenderPipeline;

  inout_stream << m_CameraPipelines.GetCount();
  for (auto it = m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
  {
    inout_stream << it.Key();
    inout_stream << it.Value();
  }

  inout_stream.EndChunk();
}

void plRenderPipelineProfileConfig::LoadRuntimeData(plChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "plRenderPipelineProfileConfig" && chunk.m_uiChunkVersion == 2)
  {
    plRenderWorld::BeginModifyCameraConfigs();
    plRenderWorld::ClearCameraConfigs();

    inout_stream >> m_sMainRenderPipeline;

    m_CameraPipelines.Clear();

    plUInt32 uiNumCamPipes = 0;
    inout_stream >> uiNumCamPipes;
    for (plUInt32 i = 0; i < uiNumCamPipes; ++i)
    {
      plString sPipeName, sPipeAsset;

      inout_stream >> sPipeName;
      inout_stream >> sPipeAsset;

      m_CameraPipelines[sPipeName] = sPipeAsset;

      plRenderWorld::CameraConfig cfg;
      cfg.m_hRenderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>(sPipeAsset);

      plRenderWorld::SetCameraConfig(sPipeName, cfg);
    }

    plRenderWorld::EndModifyCameraConfigs();
  }
}



PL_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_RendererProfileConfigs);
