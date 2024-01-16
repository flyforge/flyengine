#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace plProcGenInternal
{
  extern Pattern* GetPattern(plTempHashedString sName);
}

using namespace plProcGenInternal;

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenGraphResource, 1, plRTTIDefaultAllocator<plProcGenGraphResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plProcGenGraphResource);
// clang-format on

plProcGenGraphResource::plProcGenGraphResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plProcGenGraphResource::~plProcGenGraphResource() = default;

const plDynamicArray<plSharedPtr<const PlacementOutput>>& plProcGenGraphResource::GetPlacementOutputs() const
{
  return m_PlacementOutputs;
}

const plDynamicArray<plSharedPtr<const VertexColorOutput>>& plProcGenGraphResource::GetVertexColorOutputs() const
{
  return m_VertexColorOutputs;
}

plResourceLoadDesc plProcGenGraphResource::UnloadData(Unload WhatToUnload)
{
  m_PlacementOutputs.Clear();
  m_VertexColorOutputs.Clear();
  m_pSharedData = nullptr;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plProcGenGraphResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_LOG_BLOCK("plProcGenGraphResource::UpdateContent", GetResourceIdOrDescription());

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
  AssetHash.Read(*Stream).IgnoreResult();

  plUniquePtr<plStringDeduplicationReadContext> pStringDedupReadContext;
  if (AssetHash.GetFileVersion() >= 5)
  {
    pStringDedupReadContext = PLASMA_DEFAULT_NEW(plStringDeduplicationReadContext, *Stream);
  }

  // load
  {
    plChunkStreamReader chunk(*Stream);
    chunk.SetEndChunkFileMode(plChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    plStringBuilder sTemp;

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "SharedData")
      {
        plSharedPtr<GraphSharedData> pSharedData = PLASMA_DEFAULT_NEW(GraphSharedData);
        if (pSharedData->Load(chunk).Succeeded())
        {
          m_pSharedData = pSharedData;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "PlacementOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 4)
        {
          plLog::Error("Invalid PlacementOutputs Chunk Version {0}. Expected >= 4", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        plUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_PlacementOutputs.Reserve(uiNumOutputs);
        for (plUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          plUniquePtr<plExpressionByteCode> pByteCode = PLASMA_DEFAULT_NEW(plExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          plSharedPtr<PlacementOutput> pOutput = PLASMA_DEFAULT_NEW(PlacementOutput);
          pOutput->m_pByteCode = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices).IgnoreResult();

          plUInt64 uiNumObjectsToPlace = 0;
          chunk >> uiNumObjectsToPlace;

          for (plUInt32 uiObjectIndex = 0; uiObjectIndex < static_cast<plUInt32>(uiNumObjectsToPlace); ++uiObjectIndex)
          {
            chunk >> sTemp;
            pOutput->m_ObjectsToPlace.ExpandAndGetRef() = plResourceManager::LoadResource<plPrefabResource>(sTemp);
          }

          pOutput->m_pPattern = plProcGenInternal::GetPattern("Bayer");

          chunk >> pOutput->m_fFootprint;

          chunk >> pOutput->m_vMinOffset;
          chunk >> pOutput->m_vMaxOffset;

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 6)
          {
            chunk >> pOutput->m_YawRotationSnap;
          }
          chunk >> pOutput->m_fAlignToNormal;

          chunk >> pOutput->m_vMinScale;
          chunk >> pOutput->m_vMaxScale;

          chunk >> pOutput->m_fCullDistance;

          chunk >> pOutput->m_uiCollisionLayer;

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hColorGradient = plResourceManager::LoadResource<plColorGradientResource>(sTemp);
          }

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hSurface = plResourceManager::LoadResource<plSurfaceResource>(sTemp);
          }

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 5)
          {
            chunk >> pOutput->m_Mode;
          }

          m_PlacementOutputs.PushBack(pOutput);
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "VertexColorOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 2)
        {
          plLog::Error("Invalid VertexColorOutputs Chunk Version {0}. Expected >= 2", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        plUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_VertexColorOutputs.Reserve(uiNumOutputs);
        for (plUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          plUniquePtr<plExpressionByteCode> pByteCode = PLASMA_DEFAULT_NEW(plExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          plSharedPtr<VertexColorOutput> pOutput = PLASMA_DEFAULT_NEW(VertexColorOutput);
          pOutput->m_pByteCode = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices).IgnoreResult();

          m_VertexColorOutputs.PushBack(pOutput);
        }
      }

      chunk.NextChunk();
    }

    chunk.EndStream();
    pStringDedupReadContext = nullptr;

    // link shared data
    if (m_pSharedData != nullptr)
    {
      for (auto& pPlacementOutput : m_PlacementOutputs)
      {
        const_cast<PlacementOutput*>(pPlacementOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }

      for (auto& pVertexColorOutput : m_VertexColorOutputs)
      {
        const_cast<VertexColorOutput*>(pVertexColorOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }
    }
  }

  res.m_State = plResourceState::Loaded;
  return res;
}

void plProcGenGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plProcGenGraphResource, plProcGenGraphResourceDescriptor)
{
  // PLASMA_REPORT_FAILURE("This resource type does not support creating data.");

  // Missing resource

  auto pOutput = PLASMA_DEFAULT_NEW(PlacementOutput);
  pOutput->m_sName.Assign("MissingPlacementOutput");
  pOutput->m_ObjectsToPlace.PushBack(plResourceManager::GetResourceTypeMissingFallback<plPrefabResource>());
  pOutput->m_pPattern = plProcGenInternal::GetPattern("Bayer");
  pOutput->m_fFootprint = 3.0f;
  pOutput->m_vMinOffset.Set(-1.0f, -1.0f, -0.5f);
  pOutput->m_vMaxOffset.Set(1.0f, 1.0f, 0.0f);
  pOutput->m_vMinScale.Set(1.0f, 1.0f, 1.0f);
  pOutput->m_vMaxScale.Set(1.5f, 1.5f, 2.0f);

  m_PlacementOutputs.PushBack(pOutput);
  //

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}
