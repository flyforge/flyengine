#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Configuration/RendererProfileConfigs.h>

#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineProfileConfig, 1, plRTTIDefaultAllocator<plRenderPipelineProfileConfig>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    // MainRenderPipeline.plRenderPipelineAsset
    PLASMA_MEMBER_PROPERTY("MainRenderPipeline", m_sMainRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"))),
    // EditorRenderPipeline.plRenderPipelineAsset
    //PLASMA_MEMBER_PROPERTY("EditorRenderPipeline", m_sEditorRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"))),
    // DebugRenderPipeline.plRenderPipelineAsset
    //PLASMA_MEMBER_PROPERTY("DebugRenderPipeline", m_sDebugRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"))),

    PLASMA_MAP_MEMBER_PROPERTY("CameraPipelines", m_CameraPipelines)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline")),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plRenderPipelineProfileConfig::SaveRuntimeData(plChunkStreamWriter& stream) const
{
  stream.BeginChunk("plRenderPipelineProfileConfig", 2);

  stream << m_sMainRenderPipeline;

  stream << m_CameraPipelines.GetCount();
  for (auto it = m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
  {
    stream << it.Key();
    stream << it.Value();
  }

  stream.EndChunk();
}

void plRenderPipelineProfileConfig::LoadRuntimeData(plChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "plRenderPipelineProfileConfig" && chunk.m_uiChunkVersion == 2)
  {
    plRenderWorld::BeginModifyCameraConfigs();
    plRenderWorld::ClearCameraConfigs();

    stream >> m_sMainRenderPipeline;

    m_CameraPipelines.Clear();

    plUInt32 uiNumCamPipes = 0;
    stream >> uiNumCamPipes;
    for (plUInt32 i = 0; i < uiNumCamPipes; ++i)
    {
      plString sPipeName, sPipeAsset;

      stream >> sPipeName;
      stream >> sPipeAsset;

      m_CameraPipelines[sPipeName] = sPipeAsset;

      plRenderWorld::CameraConfig cfg;
      cfg.m_hRenderPipeline = plResourceManager::LoadResource<plRenderPipelineResource>(sPipeAsset);

      plRenderWorld::SetCameraConfig(sPipeName, cfg);
    }

    plRenderWorld::EndModifyCameraConfigs();
  }
}



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_RendererProfileConfigs);
