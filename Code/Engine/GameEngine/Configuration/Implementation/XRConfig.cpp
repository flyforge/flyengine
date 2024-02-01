#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <GameEngine/Configuration/XRConfig.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plXRConfig, 2, plRTTIDefaultAllocator<plXRConfig>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("EnableXR", m_bEnableXR),
    // HololensRenderPipeline.plRenderPipelineAsset
    PL_MEMBER_PROPERTY("XRRenderPipeline", m_sXRRenderPipeline)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_RenderPipeline"), new plDefaultValueAttribute(plStringView("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"))),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plXRConfig::SaveRuntimeData(plChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("plXRConfig", 2);

  inout_stream << m_bEnableXR;
  inout_stream << m_sXRRenderPipeline;

  inout_stream.EndChunk();
}

void plXRConfig::LoadRuntimeData(plChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "plVRConfig" && chunk.m_uiChunkVersion == 1)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
  }
  else if (chunk.m_sChunkName == "plXRConfig" && chunk.m_uiChunkVersion == 2)
  {
    inout_stream >> m_bEnableXR;
    inout_stream >> m_sXRRenderPipeline;
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
  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("plXRConfig");
    pNode->RenameProperty("EnableVR", "EnableXR");
    pNode->RenameProperty("VRRenderPipeline", "XRRenderPipeline");
  }
};

plVRConfig_1_2 g_plVRConfig_1_2;

PL_STATICLINK_FILE(GameEngine, GameEngine_Configuration_Implementation_XRConfig);
