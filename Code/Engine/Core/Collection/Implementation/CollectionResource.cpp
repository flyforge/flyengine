#include <Core/CorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Collection/CollectionResource.h>
#include <Foundation/Profiling/Profiling.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCollectionResource, 1, plRTTIDefaultAllocator<plCollectionResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plCollectionResource);

plCollectionResource::plCollectionResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

// UnloadData() already makes sure to call UnregisterNames();
plCollectionResource::~plCollectionResource() = default;

bool plCollectionResource::PreloadResources(plUInt32 uiNumResourcesToPreload)
{
  PL_LOCK(m_PreloadMutex);
  PL_PROFILE_SCOPE("Inject Resources to Preload");

  if (m_PreloadedResources.GetCount() == m_Collection.m_Resources.GetCount())
  {
    // All resources have already been queued so there is no need
    // to redo the work. Clearing the array would in fact potentially
    // trigger one of the resources to be unloaded, undoing the work
    // that was already done to preload the collection.
    return false;
  }

  m_PreloadedResources.Reserve(m_Collection.m_Resources.GetCount());

  const plUInt32 remainingResources = m_Collection.m_Resources.GetCount() - m_PreloadedResources.GetCount();
  const plUInt32 end = plMath::Min(remainingResources, uiNumResourcesToPreload) + m_PreloadedResources.GetCount();
  for (plUInt32 i = m_PreloadedResources.GetCount(); i < end; ++i)
  {
    const plCollectionEntry& e = m_Collection.m_Resources[i];
    plTypelessResourceHandle hTypeless;

    if (!e.m_sAssetTypeName.IsEmpty())
    {
      if (const plRTTI* pRtti = plResourceManager::FindResourceForAssetType(e.m_sAssetTypeName))
      {
        hTypeless = plResourceManager::LoadResourceByType(pRtti, e.m_sResourceID);
      }
      else
      {
        plLog::Error("There was no valid RTTI available for assets with type name '{}'. Could not pre-load resource '{}'. Did you forget to register "
                     "the resource type with the plResourceManager?",
          e.m_sAssetTypeName, plArgSensitive(e.m_sResourceID, "ResourceID"));
      }
    }
    else
    {
      plLog::Error("Asset '{}' had an empty asset type name. Cannot pre-load it.", plArgSensitive(e.m_sResourceID, "ResourceID"));
    }

    m_PreloadedResources.PushBack(hTypeless);

    if (hTypeless.IsValid())
    {
      plResourceManager::PreloadResource(hTypeless);
    }
  }

  return m_PreloadedResources.GetCount() < m_Collection.m_Resources.GetCount();
}

bool plCollectionResource::IsLoadingFinished(float* out_pProgress) const
{
  PL_LOCK(m_PreloadMutex);

  plUInt64 loadedWeight = 0;
  plUInt64 totalWeight = 0;

  for (plUInt32 i = 0; i < m_PreloadedResources.GetCount(); i++)
  {
    const plTypelessResourceHandle& hResource = m_PreloadedResources[i];
    if (!hResource.IsValid())
      continue;

    const plCollectionEntry& entry = m_Collection.m_Resources[i];
    plUInt64 thisWeight = plMath::Max(entry.m_uiFileSize, 1ull); // if file sizes are not specified, we weight by 1
    plResourceState state = plResourceManager::GetLoadingState(hResource);

    if (state == plResourceState::Loaded || state == plResourceState::LoadedResourceMissing)
    {
      loadedWeight += thisWeight;
    }

    if (state != plResourceState::Invalid)
    {
      totalWeight += thisWeight;
    }
  }

  if (out_pProgress != nullptr)
  {
    const float maxLoadedFraction = m_Collection.m_Resources.GetCount() == 0 ? 1.f : (float)m_PreloadedResources.GetCount() / m_Collection.m_Resources.GetCount();
    if (totalWeight != 0 && totalWeight != loadedWeight)
    {
      *out_pProgress = static_cast<float>(static_cast<double>(loadedWeight) / totalWeight) * maxLoadedFraction;
    }
    else
    {
      *out_pProgress = maxLoadedFraction;
    }
  }

  if (totalWeight == 0 || totalWeight == loadedWeight)
  {
    return true;
  }

  return false;
}


const plCollectionResourceDescriptor& plCollectionResource::GetDescriptor() const
{
  return m_Collection;
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plCollectionResource, plCollectionResourceDescriptor)
{
  m_Collection = descriptor;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

plResourceLoadDesc plCollectionResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  {
    UnregisterNames();
    // This lock unnecessary as this function is only called when the reference count is 0, i.e. if we deallocate this.
    // It is intentionally removed as it caused this lock and the resource manager lock to be locked in reverse order.
    // To prevent potential deadlocks and be able to sanity check our locking the entire codebase should never lock any
    // locks in reverse order, even if this lock is probably fine it prevents us from reasoning over the entire system.
    // PL_LOCK(m_preloadMutex);
    m_PreloadedResources.Clear();
    m_Collection.m_Resources.Clear();

    m_PreloadedResources.Compact();
    m_Collection.m_Resources.Compact();
  }

  return res;
}

plResourceLoadDesc plCollectionResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plCollectionResource::UpdateContent", GetResourceIdOrDescription());

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

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Collection.Load(*Stream);

  res.m_State = plResourceState::Loaded;
  return res;
}

void plCollectionResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  PL_LOCK(m_PreloadMutex);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<plUInt32>(m_PreloadedResources.GetHeapMemoryUsage() + m_Collection.m_Resources.GetHeapMemoryUsage());
}


void plCollectionResource::RegisterNames()
{
  if (m_bRegistered)
    return;

  m_bRegistered = true;

  PL_LOCK(plResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      plResourceManager::RegisterNamedResource(entry.m_sOptionalNiceLookupName, entry.m_sResourceID);
    }
  }
}


void plCollectionResource::UnregisterNames()
{
  if (!m_bRegistered)
    return;

  m_bRegistered = false;

  PL_LOCK(plResourceManager::GetMutex());

  for (const auto& entry : m_Collection.m_Resources)
  {
    if (!entry.m_sOptionalNiceLookupName.IsEmpty())
    {
      plResourceManager::UnregisterNamedResource(entry.m_sOptionalNiceLookupName);
    }
  }
}

void plCollectionResourceDescriptor::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 3;
  const plUInt8 uiIdentifier = 0xC0;
  const plUInt32 uiNumResources = m_Resources.GetCount();

  inout_stream << uiVersion;
  inout_stream << uiIdentifier;
  inout_stream << uiNumResources;

  for (plUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream << m_Resources[i].m_sAssetTypeName;
    inout_stream << m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream << m_Resources[i].m_sResourceID;
    inout_stream << m_Resources[i].m_uiFileSize;
  }
}

void plCollectionResourceDescriptor::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  plUInt8 uiIdentifier = 0;
  plUInt32 uiNumResources = 0;

  inout_stream >> uiVersion;
  inout_stream >> uiIdentifier;

  if (uiVersion == 1)
  {
    plUInt16 uiNumResourcesShort;
    inout_stream >> uiNumResourcesShort;
    uiNumResources = uiNumResourcesShort;
  }
  else
  {
    inout_stream >> uiNumResources;
  }

  PL_ASSERT_DEV(uiIdentifier == 0xC0, "File does not contain a valid plCollectionResourceDescriptor");
  PL_ASSERT_DEV(uiVersion > 0 && uiVersion <= 3, "Invalid file version {0}", uiVersion);

  m_Resources.SetCount(uiNumResources);

  for (plUInt32 i = 0; i < uiNumResources; ++i)
  {
    inout_stream >> m_Resources[i].m_sAssetTypeName;
    inout_stream >> m_Resources[i].m_sOptionalNiceLookupName;
    inout_stream >> m_Resources[i].m_sResourceID;
    if (uiVersion >= 3)
    {
      inout_stream >> m_Resources[i].m_uiFileSize;
    }
  }
}



PL_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionResource);
