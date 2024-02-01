#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/System/StackTracer.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plResource, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_CORE_DLL void IncreaseResourceRefCount(plResource* pResource, const void* pOwner)
{
#if PL_ENABLED(PL_RESOURCEHANDLE_STACK_TRACES)
  {
    PL_LOCK(pResource->m_HandleStackTraceMutex);

    auto& info = pResource->m_HandleStackTraces[pOwner];

    plArrayPtr<void*> ptr(info.m_Ptrs);

    info.m_uiNumPtrs = plStackTracer::GetStackTrace(ptr);
  }
#endif

  pResource->m_iReferenceCount.Increment();
}

PL_CORE_DLL void DecreaseResourceRefCount(plResource* pResource, const void* pOwner)
{
#if PL_ENABLED(PL_RESOURCEHANDLE_STACK_TRACES)
  {
    PL_LOCK(pResource->m_HandleStackTraceMutex);

    if (!pResource->m_HandleStackTraces.Remove(pOwner, nullptr))
    {
      PL_REPORT_FAILURE("No associated stack-trace!");
    }
  }
#endif

  pResource->m_iReferenceCount.Decrement();
}

#if PL_ENABLED(PL_RESOURCEHANDLE_STACK_TRACES)
PL_CORE_DLL void MigrateResourceRefCount(plResource* pResource, const void* pOldOwner, const void* pNewOwner)
{
  PL_LOCK(pResource->m_HandleStackTraceMutex);

  // allocate / resize the hash-table first to ensure the iterator stays valid
  auto& newInfo = pResource->m_HandleStackTraces[pNewOwner];

  auto it = pResource->m_HandleStackTraces.Find(pOldOwner);
  if (!it.IsValid())
  {
    PL_REPORT_FAILURE("No associated stack-trace!");
  }
  else
  {
    newInfo = it.Value();
    pResource->m_HandleStackTraces.Remove(it);
  }
}
#endif

plResource::~plResource()
{
  PL_ASSERT_DEV(!plResourceManager::IsQueuedForLoading(this), "Cannot deallocate a resource while it is still qeued for loading");
}

plResource::plResource(DoUpdate ResourceUpdateThread, plUInt8 uiQualityLevelsLoadable)
{
  m_Flags.AddOrRemove(plResourceFlags::UpdateOnMainThread, ResourceUpdateThread == DoUpdate::OnMainThread);

  m_uiQualityLevelsLoadable = uiQualityLevelsLoadable;
}

#if PL_ENABLED(PL_RESOURCEHANDLE_STACK_TRACES)
static void LogStackTrace(const char* szText)
{
  plLog::Info(szText);
};
#endif

void plResource::PrintHandleStackTraces()
{
#if PL_ENABLED(PL_RESOURCEHANDLE_STACK_TRACES)

  PL_LOCK(m_HandleStackTraceMutex);

  PL_LOG_BLOCK("Resource Handle Stack Traces");

  for (auto& it : m_HandleStackTraces)
  {
    PL_LOG_BLOCK("Handle Trace");

    plStackTracer::ResolveStackTrace(plArrayPtr<void*>(it.Value().m_Ptrs, it.Value().m_uiNumPtrs), LogStackTrace);
  }

#else

  plLog::Warning("Compile with PL_RESOURCEHANDLE_STACK_TRACES set to PL_ON to enable support for resource handle stack traces.");

#endif
}

void plResource::SetResourceDescription(plStringView sDescription)
{
  m_sResourceDescription = sDescription;
}

void plResource::SetUniqueID(plStringView sUniqueID, bool bIsReloadable)
{
  m_sUniqueID = sUniqueID;
  m_uiUniqueIDHash = plHashingUtils::StringHash(sUniqueID);
  SetIsReloadable(bIsReloadable);

  plResourceEvent e;
  e.m_pResource = this;
  e.m_Type = plResourceEvent::Type::ResourceCreated;
  plResourceManager::BroadcastResourceEvent(e);
}

void plResource::CallUnloadData(Unload WhatToUnload)
{
  PL_LOG_BLOCK("plResource::UnloadData", GetResourceID().GetData());

  plResourceEvent e;
  e.m_pResource = this;
  e.m_Type = plResourceEvent::Type::ResourceContentUnloading;
  plResourceManager::BroadcastResourceEvent(e);

  plResourceLoadDesc ld = UnloadData(WhatToUnload);

  PL_ASSERT_DEV(ld.m_State != plResourceState::Invalid, "UnloadData() did not return a valid resource load state");
  PL_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsDiscardable correctly");
  PL_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UnloadData() did not fill out m_uiQualityLevelsLoadable correctly");

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
}

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
thread_local const plResource* g_pCurrentlyUpdatingContent = nullptr;

const plResource* plResource::GetCurrentlyUpdatingContent()
{
  return g_pCurrentlyUpdatingContent;
}
#endif

void plResource::CallUpdateContent(plStreamReader* Stream)
{
  PL_PROFILE_SCOPE("CallUpdateContent");

  PL_LOG_BLOCK("plResource::UpdateContent", GetResourceID().GetData());

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  const plResource* pPreviouslyUpdatingContent = g_pCurrentlyUpdatingContent;
  g_pCurrentlyUpdatingContent = this;
  plResourceLoadDesc ld = UpdateContent(Stream);
  g_pCurrentlyUpdatingContent = pPreviouslyUpdatingContent;
#else
  plResourceLoadDesc ld = UpdateContent(Stream);
#endif

  PL_ASSERT_DEV(ld.m_State != plResourceState::Invalid, "UpdateContent() did not return a valid resource load state");
  PL_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsDiscardable correctly");
  PL_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "UpdateContent() did not fill out m_uiQualityLevelsLoadable correctly");

  if (ld.m_State == plResourceState::LoadedResourceMissing)
  {
    ReportResourceIsMissing();
  }

  IncResourceChangeCounter();

  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;
  m_LoadingState = ld.m_State;

  plResourceEvent e;
  e.m_pResource = this;
  e.m_Type = plResourceEvent::Type::ResourceContentUpdated;
  plResourceManager::BroadcastResourceEvent(e);

  plLog::Debug("Updated {0} - '{1}'", GetDynamicRTTI()->GetTypeName(), plArgSensitive(GetResourceDescription(), "ResourceDesc"));
}

float plResource::GetLoadingPriority(plTime now) const
{
  if (m_Priority == plResourcePriority::Critical)
    return 0.0f;

  // low priority values mean it gets loaded earlier
  float fPriority = static_cast<float>(m_Priority) * 10.0f;

  if (GetLoadingState() == plResourceState::Loaded)
  {
    // already loaded -> more penalty
    fPriority += 30.0f;

    // the more it could discard, the less important it is to load more of it
    fPriority += GetNumQualityLevelsDiscardable() * 10.0f;
  }
  else
  {
    const plBitflags<plResourceFlags> flags = GetBaseResourceFlags();

    if (flags.IsAnySet(plResourceFlags::ResourceHasFallback))
    {
      // if the resource has a very specific fallback, it is least important to be get loaded
      fPriority += 20.0f;
    }
    else if (flags.IsAnySet(plResourceFlags::ResourceHasTypeFallback))
    {
      // if it has at least a type fallback, it is less important to get loaded
      fPriority += 10.0f;
    }
  }

  // everything acquired in the last N seconds gets a higher priority
  // by getting the lowest penalty
  const float secondsSinceAcquire = (float)(now - GetLastAcquireTime()).GetSeconds();
  const float fTimePriority = plMath::Min(10.0f, secondsSinceAcquire);

  return fPriority + fTimePriority;
}

void plResource::SetPriority(plResourcePriority priority)
{
  if (m_Priority == priority)
    return;

  m_Priority = priority;

  plResourceEvent e;
  e.m_pResource = this;
  e.m_Type = plResourceEvent::Type::ResourcePriorityChanged;
  plResourceManager::BroadcastResourceEvent(e);
}

plResourceTypeLoader* plResource::GetDefaultResourceTypeLoader() const
{
  return plResourceManager::GetDefaultResourceLoader();
}

void plResource::ReportResourceIsMissing()
{
  plLog::SeriousWarning("Missing Resource of Type '{2}': '{0}' ('{1}')", plArgSensitive(GetResourceID(), "ResourceID"),
    plArgSensitive(m_sResourceDescription, "ResourceDesc"), GetDynamicRTTI()->GetTypeName());
}

void plResource::VerifyAfterCreateResource(const plResourceLoadDesc& ld)
{
  PL_ASSERT_DEV(ld.m_State != plResourceState::Invalid, "CreateResource() did not return a valid resource load state");
  PL_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsDiscardable correctly");
  PL_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsLoadable correctly");

  IncResourceChangeCounter();

  m_LoadingState = ld.m_State;
  m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
  m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;

  /* Update Memory Usage*/
  {
    plResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    UpdateMemoryUsage(MemUsage);

    PL_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", GetResourceID());
    PL_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", GetResourceID());

    m_MemoryUsage = MemUsage;
  }

  plResourceEvent e;
  e.m_pResource = this;
  e.m_Type = plResourceEvent::Type::ResourceContentUpdated;
  plResourceManager::BroadcastResourceEvent(e);

  plLog::Debug("Created {0} - '{1}' ", GetDynamicRTTI()->GetTypeName(), plArgSensitive(GetResourceDescription(), "ResourceDesc"));
}

PL_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_Resource);
