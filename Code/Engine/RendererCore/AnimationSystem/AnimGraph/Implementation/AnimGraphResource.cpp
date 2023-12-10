#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationClipMapping, 1, plRTTIDefaultAllocator<plAnimationClipMapping>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("ClipName", GetClipName, SetClipName)->AddAttributes(new plDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    PLASMA_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
  }
    PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimGraphResource, 1, plRTTIDefaultAllocator<plAnimGraphResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plAnimGraphResource);
// clang-format on

const char* plAnimationClipMapping::GetClip() const
{
  if (m_hClip.IsValid())
    return m_hClip.GetResourceID();

  return "";
}

void plAnimationClipMapping::SetClip(const char* szName)
{
  plAnimationClipResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szName))
  {
    hResource = plResourceManager::LoadResource<plAnimationClipResource>(szName);
  }

  m_hClip = hResource;
}

plAnimGraphResource::plAnimGraphResource()
  : plResource(plResource::DoUpdate::OnAnyThread, 0)
{
}

plAnimGraphResource::~plAnimGraphResource() = default;

plResourceLoadDesc plAnimGraphResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc d;
  d.m_State = plResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

plResourceLoadDesc plAnimGraphResource::UpdateContent(plStreamReader* Stream)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).AssertSuccess();

  {
    const auto uiVersion = Stream->ReadVersion(2);
    Stream->ReadArray(m_IncludeGraphs).AssertSuccess();

    if (uiVersion >= 2)
    {
      plUInt32 uiNum = 0;
      *Stream >> uiNum;

      m_AnimationClipMapping.SetCount(uiNum);
      for (plUInt32 i = 0; i < uiNum; ++i)
      {
        *Stream >> m_AnimationClipMapping[i].m_sClipName;
        *Stream >> m_AnimationClipMapping[i].m_hClip;
      }
    }
  }

  if (m_AnimGraph.Deserialize(*Stream).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  m_AnimGraph.PrepareForUse();

  res.m_State = plResourceState::Loaded;

  return res;
}

void plAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
