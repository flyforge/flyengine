#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Configuration/XRConfig.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plXRConfig, 2, plRTTIDefaultAllocator<plXRConfig>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("EnableXR", m_bEnableXR),
    // HololensRenderPipeline.plRenderPipelineAsset
    PLASMA_MEMBER_PROPERTY("XRRenderPipeline", m_sXRRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"))),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plXRConfig::SaveRuntimeData(plChunkStreamWriter& stream) const
{
  stream.BeginChunk("plXRConfig", 2);

  stream << m_bEnableXR;
  stream << m_sXRRenderPipeline;

  stream.EndChunk();
}

void plXRConfig::LoadRuntimeData(plChunkStreamReader& stream)
{
  const auto& chunk = stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "plVRConfig" && chunk.m_uiChunkVersion == 1)
  {
    stream >> m_bEnableXR;
    stream >> m_sXRRenderPipeline;
  }
  else if (chunk.m_sChunkName == "plXRConfig" && chunk.m_uiChunkVersion == 2)
  {
    stream >> m_bEnableXR;
    stream >> m_sXRRenderPipeline;
  }
}


//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plVRConfig_1_2 : public plGraphPatch
{
public:
  plVRConfig_1_2()
    : plGraphPatch("plVRConfig", 5)
  {
  }
  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    context.RenameClass("plXRConfig");
    pNode->RenameProperty("EnableVR", "EnableXR");
    pNode->RenameProperty("VRRenderPipeline", "XRRenderPipeline");
  }
};

plVRConfig_1_2 g_plVRConfig_1_2;

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_XRConfig);
