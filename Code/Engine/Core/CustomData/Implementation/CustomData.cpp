#include <Core/CorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/CustomData/CustomData.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCustomData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plCustomData::Load(plAbstractObjectGraph& ref_graph, plRttiConverterContext& ref_context, const plAbstractObjectNode* pRootNode)
{
  plRttiConverterReader convRead(&ref_graph, &ref_context);
  convRead.ApplyPropertiesToObject(pRootNode, GetDynamicRTTI(), this);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCustomDataResourceBase, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCustomDataResourceBase::plCustomDataResourceBase()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plCustomDataResourceBase::~plCustomDataResourceBase() = default;

plResourceLoadDesc plCustomDataResourceBase::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;
  return res;
}

plResourceLoadDesc plCustomDataResourceBase::UpdateContent_Internal(plStreamReader* Stream, const plRTTI& rtti)
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
  AssetHash.Read(*Stream).IgnoreResult();

  plAbstractObjectGraph graph;
  plRttiConverterContext context;

  plAbstractGraphBinarySerializer::Read(*Stream, &graph);

  const plAbstractObjectNode* pRootNode = graph.GetNodeByName("root");

  if (pRootNode != nullptr && pRootNode->GetType() != rtti.GetTypeName())
  {
    plLog::Error("Expected plCustomData type '{}' but resource is of type '{}' ('{}')", rtti.GetTypeName(), pRootNode->GetType(), GetResourceIdOrDescription());

    // make sure we create a default-initialized object and don't deserialize data that happens to match
    pRootNode = nullptr;
  }

  CreateAndLoadData(graph, context, pRootNode);

  res.m_State = plResourceState::Loaded;
  return res;
}

PLASMA_STATICLINK_FILE(Core, Core_Utils_Implementation_CustomData);