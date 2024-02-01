#include <Core/CorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPrefabResource, 1, plRTTIDefaultAllocator<plPrefabResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plPrefabResource);
// clang-format on

plPrefabResource::plPrefabResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

void plPrefabResource::InstantiatePrefab(plWorld& ref_world, const plTransform& rootTransform, plPrefabInstantiationOptions options, const plArrayMap<plHashedString, plVariant>* pExposedParamValues)
{
  if (GetLoadingState() != plResourceState::Loaded)
    return;

  if (pExposedParamValues != nullptr && !pExposedParamValues->IsEmpty())
  {
    plHybridArray<plGameObject*, 8> createdRootObjects;
    plHybridArray<plGameObject*, 8> createdChildObjects;

    if (options.m_pCreatedRootObjectsOut == nullptr)
    {
      options.m_pCreatedRootObjectsOut = &createdRootObjects;
    }

    if (options.m_pCreatedChildObjectsOut == nullptr)
    {
      options.m_pCreatedChildObjectsOut = &createdChildObjects;
    }

    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);

    PL_ASSERT_DEBUG(options.m_pCreatedRootObjectsOut != options.m_pCreatedChildObjectsOut, "These pointers must point to different arrays, otherwise applying exposed properties doesn't work correctly.");
    ApplyExposedParameterValues(pExposedParamValues, *options.m_pCreatedChildObjectsOut, *options.m_pCreatedRootObjectsOut);
  }
  else
  {
    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);
  }
}

plPrefabResource::InstantiateResult plPrefabResource::InstantiatePrefab(const plPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, plWorld& ref_world, const plTransform& rootTransform, plPrefabInstantiationOptions options, const plArrayMap<plHashedString, plVariant>* pExposedParamValues /*= nullptr*/)
{
  plResourceLock<plPrefabResource> pPrefab(hPrefab, bBlockTillLoaded ? plResourceAcquireMode::BlockTillLoaded_NeverFail : plResourceAcquireMode::AllowLoadingFallback_NeverFail);

  switch (pPrefab.GetAcquireResult())
  {
    case plResourceAcquireResult::Final:
      pPrefab->InstantiatePrefab(ref_world, rootTransform, options, pExposedParamValues);
      return InstantiateResult::Success;

    case plResourceAcquireResult::LoadingFallback:
      return InstantiateResult::NotYetLoaded;

    default:
      return InstantiateResult::Error;
  }
}

void plPrefabResource::ApplyExposedParameterValues(const plArrayMap<plHashedString, plVariant>* pExposedParamValues, const plDynamicArray<plGameObject*>& createdChildObjects, const plDynamicArray<plGameObject*>& createdRootObjects) const
{
  const plUInt32 uiNumParamDescs = m_PrefabParamDescs.GetCount();

  for (plUInt32 i = 0; i < pExposedParamValues->GetCount(); ++i)
  {
    const plHashedString& name = pExposedParamValues->GetKey(i);
    const plUInt64 uiNameHash = name.GetHash();

    for (plUInt32 uiCurParam = FindFirstParamWithName(uiNameHash); uiCurParam < uiNumParamDescs; ++uiCurParam)
    {
      const auto& ppd = m_PrefabParamDescs[uiCurParam];

      if (ppd.m_sExposeName.GetHash() != uiNameHash)
        break;

      plGameObject* pTarget = ppd.m_uiWorldReaderChildObject ? createdChildObjects[ppd.m_uiWorldReaderObjectIndex] : createdRootObjects[ppd.m_uiWorldReaderObjectIndex];

      if (ppd.m_CachedPropertyPath.IsValid())
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.SetValue(pTarget, pExposedParamValues->GetValue(i));
        }
        else
        {
          for (plComponent* pComp : pTarget->GetComponents())
          {
            const plRTTI* pRtti = pComp->GetDynamicRTTI();

            // TODO: use component index instead
            // atm if the same component type is attached multiple times, they will all get the value applied
            if (pRtti->GetTypeNameHash() == ppd.m_sComponentType.GetHash())
            {
              ppd.m_CachedPropertyPath.SetValue(pComp, pExposedParamValues->GetValue(i));
            }
          }
        }
      }

      // Allow to bind multiple properties to the same exposed parameter name
      // Therefore, do not break here, but continue iterating
    }
  }
}

plResourceLoadDesc plPrefabResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  if (WhatToUnload == plResource::Unload::AllQualityLevels)
  {
    m_WorldReader.ClearAndCompact();
  }

  return res;
}

plResourceLoadDesc plPrefabResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plPrefabResource::UpdateContent", GetResourceIdOrDescription());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  plStreamReader& s = *Stream;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plString sAbsFilePath;
    s >> sAbsFilePath;
  }

  plAssetFileHeader assetHeader;
  assetHeader.Read(s).IgnoreResult();

  char szSceneTag[16];
  s.ReadBytes(szSceneTag, sizeof(char) * 16);
  PL_ASSERT_DEV(plStringUtils::IsEqualN(szSceneTag, "[plBinaryScene]", 16), "The given file is not a valid prefab file");

  if (!plStringUtils::IsEqualN(szSceneTag, "[plBinaryScene]", 16))
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  m_WorldReader.ReadWorldDescription(s).IgnoreResult();

  if (assetHeader.GetFileVersion() >= 4)
  {
    plUInt32 uiExposedParams = 0;

    s >> uiExposedParams;

    m_PrefabParamDescs.SetCount(uiExposedParams);

    for (plUInt32 i = 0; i < uiExposedParams; ++i)
    {
      auto& ppd = m_PrefabParamDescs[i];

      PL_ASSERT_DEV(assetHeader.GetFileVersion() >= 6, "Old resource version not supported anymore");
      ppd.Load(s);

      // initialize the cached property path here once
      // so we can only apply it later as often as needed
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.InitializeFromPath(*plGetStaticRTTI<plGameObject>(), ppd.m_sProperty).IgnoreResult();
        }
        else
        {
          if (const plRTTI* pRtti = plRTTI::FindTypeByNameHash(ppd.m_sComponentType.GetHash()))
          {
            ppd.m_CachedPropertyPath.InitializeFromPath(*pRtti, ppd.m_sProperty).IgnoreResult();
          }
        }
      }
    }

    // sort exposed parameter descriptions by name hash for quicker access
    m_PrefabParamDescs.Sort([](const plExposedPrefabParameterDesc& lhs, const plExposedPrefabParameterDesc& rhs) -> bool { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });
  }

  res.m_State = plResourceState::Loaded;
  return res;
}

void plPrefabResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_WorldReader.GetHeapMemoryUsage() + sizeof(this);
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plPrefabResource, plPrefabResourceDescriptor)
{
  plResourceLoadDesc desc;
  desc.m_State = plResourceState::Loaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable = 0;
  return desc;
}

plUInt32 plPrefabResource::FindFirstParamWithName(plUInt64 uiNameHash) const
{
  plUInt32 lb = 0;
  plUInt32 ub = m_PrefabParamDescs.GetCount();

  while (lb < ub)
  {
    const plUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_PrefabParamDescs[middle].m_sExposeName.GetHash() < uiNameHash)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  return lb;
}

void plExposedPrefabParameterDesc::Save(plStreamWriter& inout_stream) const
{
  plUInt32 comb = m_uiWorldReaderObjectIndex | (m_uiWorldReaderChildObject << 31);

  inout_stream << m_sExposeName;
  inout_stream << comb;
  inout_stream << m_sComponentType;
  inout_stream << m_sProperty;
}

void plExposedPrefabParameterDesc::Load(plStreamReader& inout_stream)
{
  plUInt32 comb = 0;

  inout_stream >> m_sExposeName;
  inout_stream >> comb;
  inout_stream >> m_sComponentType;
  inout_stream >> m_sProperty;

  m_uiWorldReaderObjectIndex = comb & 0x7FFFFFFF;
  m_uiWorldReaderChildObject = (comb >> 31);
}

PL_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabResource);
