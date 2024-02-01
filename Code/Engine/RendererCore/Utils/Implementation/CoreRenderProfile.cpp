#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/Utils/CoreRenderProfile.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCoreRenderProfileConfig, 1, plRTTIDefaultAllocator<plCoreRenderProfileConfig>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ShadowAtlasTextureSize", m_uiShadowAtlasTextureSize)->AddAttributes(new plDefaultValueAttribute(4096), new plClampValueAttribute(512, 8192)),
    PL_MEMBER_PROPERTY("MaxShadowMapSize", m_uiMaxShadowMapSize)->AddAttributes(new plDefaultValueAttribute(1024), new plClampValueAttribute(64, 4096)),
    PL_MEMBER_PROPERTY("MinShadowMapSize", m_uiMinShadowMapSize)->AddAttributes(new plDefaultValueAttribute(64), new plClampValueAttribute(8, 512)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plCoreRenderProfileConfig::SaveRuntimeData(plChunkStreamWriter& inout_stream) const
{
  inout_stream.BeginChunk("plCoreRenderProfileConfig", 1);

  inout_stream << m_uiShadowAtlasTextureSize;
  inout_stream << m_uiMaxShadowMapSize;
  inout_stream << m_uiMinShadowMapSize;

  inout_stream.EndChunk();
}

void plCoreRenderProfileConfig::LoadRuntimeData(plChunkStreamReader& inout_stream)
{
  const auto& chunk = inout_stream.GetCurrentChunk();

  if (chunk.m_sChunkName == "plCoreRenderProfileConfig" && chunk.m_uiChunkVersion >= 1)
  {
    inout_stream >> m_uiShadowAtlasTextureSize;
    inout_stream >> m_uiMaxShadowMapSize;
    inout_stream >> m_uiMinShadowMapSize;
  }
}
