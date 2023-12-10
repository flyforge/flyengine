#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

/// \todo Do not unload resources while they are acquired
/// \todo Resource Type Memory Thresholds
/// \todo Preload does not load all quality levels

/// Infos to Display:
///   Ref Count (max)
///   Fallback: Type / Instance
///   Loading Time

/// Resource Flags:
/// Category / Group (Texture Sets)

/// Resource Loader
///   Requires No File Access -> on non-File Thread

plUniquePtr<plResourceManagerState> plResourceManager::s_pState;
plMutex plResourceManager::s_ResourceMutex;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Core, ResourceManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::OnCoreStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::OnCoreShutdown();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plResourceManager::OnEngineShutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on


plResourceTypeLoader* plResourceManager::GetResourceTypeLoader(const plRTTI* pRTTI)
{
  return s_pState->m_ResourceTypeLoader[pRTTI];
}

plMap<const plRTTI*, plResourceTypeLoader*>& plResourceManager::GetResourceTypeLoaders()
{
  return s_pState->m_ResourceTypeLoader;
}

void plResourceManager::AddResourceCleanupCallback(ResourceCleanupCB cb)
{
  PLASMA_ASSERT_DEV(cb.IsComparable(), "Delegates with captures are not allowed");

  for (plUInt32 i = 0; i < s_pState->m_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_pState->m_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
      return;
  }

  s_pState->m_ResourceCleanupCallbacks.PushBack(cb);
}

void plResourceManager::ClearResourceCleanupCallback(ResourceCleanupCB cb)
{
  for (plUInt32 i = 0; i < s_pState->m_ResourceCleanupCallbacks.GetCount(); ++i)
  {
    if (s_pState->m_ResourceCleanupCallbacks[i].IsEqualIfComparable(cb))
    {
      s_pState->m_ResourceCleanupCallbacks.RemoveAtAndSwap(i);
      return;
    }
  }
}

void plResourceManager::ExecuteAllResourceCleanupCallbacks()
{
  if (s_pState == nullptr)
  {
    // In case resource manager wasn't initialized, nothing to do
    return;
  }

  plDynamicArray<ResourceCleanupCB> callbacks = s_pState->m_ResourceCleanupCallbacks;
  s_pState->m_ResourceCleanupCallbacks.Clear();

  for (auto& cb : callbacks)
  {
    cb();
  }

  PLASMA_ASSERT_DEV(s_pState->m_ResourceCleanupCallbacks.IsEmpty(), "During resource cleanup, new resource cleanup callbacks were registered.");
}

plMap<const plRTTI*, plResourcePriority>& plResourceManager::GetResourceTypePriorities()
{
  return s_pState->m_ResourceTypePriorities;
}

void plResourceManager::BroadcastResourceEvent(const plResourceEvent& e)
{
  PLASMA_LOCK(s_ResourceMutex);

  // broadcast it through the resource to everyone directly interested in that specific resource
  e.m_pResource->m_ResourceEvents.Broadcast(e);

  // and then broadcast it to everyone else through the general event
  s_pState->m_ResourceEvents.Broadcast(e);
}

void plResourceManager::RegisterResourceForAssetType(plStringView sAssetTypeName, const plRTTI* pResourceType)
{
  plStringBuilder s = sAssetTypeName;
  s.ToLower();

  s_pState->m_AssetToResourceType[s] = pResourceType;
}

const plRTTI* plResourceManager::FindResourceForAssetType(plStringView sAssetTypeName)
{
  plStringBuilder s = sAssetTypeName;
  s.ToLower();

  return s_pState->m_AssetToResourceType.GetValueOrDefault(s, nullptr);
}

void plResourceManager::ForceNoFallbackAcquisition(plUInt32 uiNumFrames /*= 0xFFFFFFFF*/)
{
  s_pState->m_uiForceNoFallbackAcquisition = plMath::Max(s_pState->m_uiForceNoFallbackAcquisition, uiNumFrames);
}

plUInt32 plResourceManager::FreeAllUnusedResources()
{
  PLASMA_LOG_BLOCK("plResourceManager::FreeAllUnusedResources");

  PLASMA_PROFILE_SCOPE("FreeAllUnusedResources");

  if (s_pState == nullptr)
  {
    // In case resource manager wasn't initialized, no resources to unload
    return 0;
  }

  const bool bFreeAllUnused = true;

  plUInt32 uiUnloaded = 0;
  bool bUnloadedAny = false;
  bool bAnyFailed = false;

  do
  {
    {
      PLASMA_LOCK(s_ResourceMutex);

      bUnloadedAny = false;

      for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
      {
        LoadedResources& lr = itType.Value();

        for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); /* empty */)
        {
          plResource* pReference = it.Value();

          if (pReference->m_iReferenceCount == 0)
          {
            bUnloadedAny = true; // make sure to try again, even if DeallocateResource() fails; need to release our lock for that to prevent dead-locks

            if (DeallocateResource(pReference).Succeeded())
            {
              ++uiUnloaded;

              it = lr.m_Resources.Remove(it);
              continue;
            }
            else
            {
              bAnyFailed = true;
            }
          }

          ++it;
        }
      }
    }

    if (bAnyFailed)
    {
      // When this happens, it is possible that the resource that failed to be deleted
      // is dependent on a task that needs to be executed on THIS thread (main thread).
      // Therefore, help executing some tasks here, to unblock the task system.

      bAnyFailed = false;

      plInt32 iHelpExecTasksRounds = 1;
      plTaskSystem::WaitForCondition([&iHelpExecTasksRounds]() { return iHelpExecTasksRounds-- <= 0; });
    }

  } while (bFreeAllUnused && bUnloadedAny);

  return uiUnloaded;
}

plUInt32 plResourceManager::FreeUnusedResources(plTime timeout, plTime lastAcquireThreshold)
{
  if (timeout.IsZeroOrNegative())
    return 0;

  PLASMA_LOCK(s_ResourceMutex);
  PLASMA_LOG_BLOCK("plResourceManager::FreeUnusedResources");
  PLASMA_PROFILE_SCOPE("FreeUnusedResources");

  auto itResourceType = s_pState->m_LoadedResources.Find(s_pState->m_pFreeUnusedLastType);
  if (!itResourceType.IsValid())
  {
    itResourceType = s_pState->m_LoadedResources.GetIterator();
  }

  if (!itResourceType.IsValid())
    return 0;

  auto itResourceID = itResourceType.Value().m_Resources.Find(s_pState->m_sFreeUnusedLastResourceID);
  if (!itResourceID.IsValid())
  {
    itResourceID = itResourceType.Value().m_Resources.GetIterator();
  }

  const plTime tStart = plTime::Now();

  plUInt32 uiDeallocatedCount = 0;

  plStringBuilder sResourceName;

  const plRTTI* pLastTypeCheck = nullptr;

  // stop once we wasted enough time
  while (plTime::Now() - tStart < timeout)
  {
    if (!itResourceID.IsValid())
    {
      // reached the end of this resource type
      // advance to the next resource type
      ++itResourceType;

      if (!itResourceType.IsValid())
      {
        // if we reached the end, reset everything and stop

        s_pState->m_pFreeUnusedLastType = nullptr;
        s_pState->m_sFreeUnusedLastResourceID = plTempHashedString();
        return uiDeallocatedCount;
      }


      // reset resource ID to the beginning of this type and start over
      itResourceID = itResourceType.Value().m_Resources.GetIterator();
      continue;
    }

    s_pState->m_pFreeUnusedLastType = itResourceType.Key();
    s_pState->m_sFreeUnusedLastResourceID = itResourceID.Key();

    if (pLastTypeCheck != itResourceType.Key())
    {
      pLastTypeCheck = itResourceType.Key();

      if (GetResourceTypeInfo(pLastTypeCheck).m_bIncrementalUnload == false)
      {
        itResourceID = itResourceType.Value().m_Resources.GetEndIterator();
        continue;
      }
    }

    plResource* pResource = itResourceID.Value();

    if ((pResource->GetReferenceCount() == 0) && (tStart - pResource->GetLastAcquireTime() > lastAcquireThreshold))
    {
      sResourceName = pResource->GetResourceID();

      if (DeallocateResource(pResource).Succeeded())
      {
        plLog::Debug("Freed '{}'", plArgSensitive(sResourceName, "ResourceID"));

        ++uiDeallocatedCount;
        itResourceID = itResourceType.Value().m_Resources.Remove(itResourceID);
        continue;
      }
    }

    ++itResourceID;
  }

  return uiDeallocatedCount;
}

void plResourceManager::SetAutoFreeUnused(plTime timeout, plTime lastAcquireThreshold)
{
  s_pState->m_AutoFreeUnusedTimeout = timeout;
  s_pState->m_AutoFreeUnusedThreshold = lastAcquireThreshold;
}

void plResourceManager::AllowResourceTypeAcquireDuringUpdateContent(const plRTTI* pTypeBeingUpdated, const plRTTI* pTypeItWantsToAcquire)
{
  auto& info = s_pState->m_TypeInfo[pTypeBeingUpdated];

  PLASMA_ASSERT_DEV(info.m_bAllowNestedAcquireCached == false, "AllowResourceTypeAcquireDuringUpdateContent for type '{}' must be called before the resource info has been requested.", pTypeBeingUpdated->GetTypeName());

  if (info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) == plInvalidIndex)
  {
    info.m_NestedTypes.PushBack(pTypeItWantsToAcquire);
  }
}

bool plResourceManager::IsResourceTypeAcquireDuringUpdateContentAllowed(const plRTTI* pTypeBeingUpdated, const plRTTI* pTypeItWantsToAcquire)
{
  PLASMA_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "");

  auto& info = s_pState->m_TypeInfo[pTypeBeingUpdated];

  if (!info.m_bAllowNestedAcquireCached)
  {
    info.m_bAllowNestedAcquireCached = true;

    plSet<const plRTTI*> visited;
    plSet<const plRTTI*> todo;
    plSet<const plRTTI*> deps;

    for (const plRTTI* pRtti : info.m_NestedTypes)
    {
      plRTTI::ForEachDerivedType(pRtti, [&](const plRTTI* pDerived) { todo.Insert(pDerived); });
    }

    while (!todo.IsEmpty())
    {
      auto it = todo.GetIterator();
      const plRTTI* pRtti = it.Key();
      todo.Remove(it);

      if (visited.Contains(pRtti))
        continue;

      visited.Insert(pRtti);
      deps.Insert(pRtti);

      for (const plRTTI* pNestedRtti : s_pState->m_TypeInfo[pRtti].m_NestedTypes)
      {
        if (!visited.Contains(pNestedRtti))
        {
          plRTTI::ForEachDerivedType(pNestedRtti, [&](const plRTTI* pDerived) { todo.Insert(pDerived); });
        }
      }
    }

    info.m_NestedTypes.Clear();
    for (const plRTTI* pRtti : deps)
    {
      info.m_NestedTypes.PushBack(pRtti);
    }
    info.m_NestedTypes.Sort();
  }

  return info.m_NestedTypes.IndexOf(pTypeItWantsToAcquire) != plInvalidIndex;
}

plResult plResourceManager::DeallocateResource(plResource* pResource)
{
  //PLASMA_ASSERT_DEBUG(pResource->m_iLockCount == 0, "Resource '{0}' has a refcount of zero, but is still in an acquired state.", pResource->GetResourceID());

  if (RemoveFromLoadingQueue(pResource).Failed())
  {
    // cannot deallocate resources that are currently queued for loading,
    // especially when they are already picked up by a task
    return PLASMA_FAILURE;
  }

  pResource->CallUnloadData(plResource::Unload::AllQualityLevels);

  PLASMA_ASSERT_DEBUG(pResource->GetLoadingState() <= plResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.", pResource->GetResourceID());

  // broadcast that we are going to delete the resource
  {
    plResourceEvent e;
    e.m_pResource = pResource;
    e.m_Type = plResourceEvent::Type::ResourceDeleted;
    plResourceManager::BroadcastResourceEvent(e);
  }

  // delete the resource via the RTTI provided allocator
  pResource->GetDynamicRTTI()->GetAllocator()->Deallocate(pResource);

  return PLASMA_SUCCESS;
}

// To allow triggering this event without a link dependency
// Used by Fileserve, to trigger this event, even though Fileserve should not have a link dependency on Core
PLASMA_ON_GLOBAL_EVENT(plResourceManager_ReloadAllResources)
{
  plResourceManager::ReloadAllResources(false);
}
void plResourceManager::ResetAllResources()
{
  PLASMA_LOCK(s_ResourceMutex);
  PLASMA_LOG_BLOCK("plResourceManager::ReloadAllResources");

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      plResource* pResource = it.Value();
      pResource->ResetResource();
    }
  }
}

void plResourceManager::PerFrameUpdate()
{
  PLASMA_PROFILE_SCOPE("plResourceManagerUpdate");

  s_pState->m_LastFrameUpdate = plTime::Now();

  if (s_pState->m_bBroadcastExistsEvent)
  {
    PLASMA_LOCK(s_ResourceMutex);

    s_pState->m_bBroadcastExistsEvent = false;

    for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
    {
      for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        plResourceEvent e;
        e.m_Type = plResourceEvent::Type::ResourceExists;
        e.m_pResource = it.Value();

        plResourceManager::BroadcastResourceEvent(e);
      }
    }
  }

  {
    PLASMA_LOCK(s_ResourceMutex);

    for (auto it = s_pState->m_ResourcesToUnloadOnMainThread.GetIterator(); it.IsValid(); it.Next())
    {
      // Identify the container of loaded resource for the type of resource we want to unload.
      LoadedResources loadedResourcesForType;
      if (s_pState->m_LoadedResources.TryGetValue(it.Value(), loadedResourcesForType) == false)
      {
        continue;
      }

      // See, if the resource we want to unload still exists.
      plResource* resourceToUnload = nullptr;

      if (loadedResourcesForType.m_Resources.TryGetValue(it.Key(), resourceToUnload) == false)
      {
        continue;
      }

      PLASMA_ASSERT_DEV(resourceToUnload != nullptr, "Found a resource above, should not be nullptr.");

      // If the resource was still loaded, we are going to unload it now.
      resourceToUnload->CallUnloadData(plResource::Unload::AllQualityLevels);

      PLASMA_ASSERT_DEV(resourceToUnload->GetLoadingState() <= plResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.", resourceToUnload->GetResourceID());
    }

    s_pState->m_ResourcesToUnloadOnMainThread.Clear();
  }

  if (s_pState->m_AutoFreeUnusedTimeout.IsPositive())
  {
    FreeUnusedResources(s_pState->m_AutoFreeUnusedTimeout, s_pState->m_AutoFreeUnusedThreshold);
  }
}

const plEvent<const plResourceEvent&, plMutex>& plResourceManager::GetResourceEvents()
{
  return s_pState->m_ResourceEvents;
}

const plEvent<const plResourceManagerEvent&, plMutex>& plResourceManager::GetManagerEvents()
{
  return s_pState->m_ManagerEvents;
}

void plResourceManager::BroadcastExistsEvent()
{
  s_pState->m_bBroadcastExistsEvent = true;
}

void plResourceManager::PluginEventHandler(const plPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case plPluginEvent::AfterStartupShutdown:
    {
      // unload all resources until there are no more that can be unloaded
      // this is to prevent having resources allocated that came from a dynamic plugin
      FreeAllUnusedResources();
    }
    break;

    default:
      break;
  }
}

void plResourceManager::OnCoreStartup()
{
  s_pState = PLASMA_DEFAULT_NEW(plResourceManagerState);

  PLASMA_LOCK(s_ResourceMutex);
  s_pState->m_bAllowLaunchDataLoadTask = true;
  s_pState->m_bShutdown = false;

  plPlugin::Events().AddEventHandler(PluginEventHandler);
}

void plResourceManager::EngineAboutToShutdown()
{
  {
    PLASMA_LOCK(s_ResourceMutex);

    if (s_pState == nullptr)
    {
      // In case resource manager wasn't initialized, nothing to do
      return;
    }

    s_pState->m_bAllowLaunchDataLoadTask = false; // prevent a new one from starting
    s_pState->m_bShutdown = true;
  }

  for (plUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
  {
    plTaskSystem::CancelTask(s_pState->m_WorkerTasksDataLoad[i].m_pTask).IgnoreResult();
  }

  for (plUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    plTaskSystem::CancelTask(s_pState->m_WorkerTasksUpdateContent[i].m_pTask).IgnoreResult();
  }

  {
    PLASMA_LOCK(s_ResourceMutex);

    for (auto entry : s_pState->m_LoadingQueue)
    {
      entry.m_pResource->m_Flags.Remove(plResourceFlags::IsQueuedForLoading);
    }

    s_pState->m_LoadingQueue.Clear();

    // Since we just canceled all loading tasks above and cleared the loading queue,
    // some resources may still be flagged as 'loading', but can never get loaded.
    // That can deadlock the 'FreeAllUnused' function, because it won't delete 'loading' resources.
    // Therefore we need to make sure no resource has the IsQueuedForLoading flag set anymore.
    for (auto itTypes : s_pState->m_LoadedResources)
    {
      for (auto itRes : itTypes.Value().m_Resources)
      {
        plResource* pRes = itRes.Value();

        if (pRes->GetBaseResourceFlags().IsSet(plResourceFlags::IsQueuedForLoading))
        {
          pRes->m_Flags.Remove(plResourceFlags::IsQueuedForLoading);
        }
      }
    }
  }
}

bool plResourceManager::IsAnyLoadingInProgress()
{
  PLASMA_LOCK(s_ResourceMutex);

  if (s_pState->m_LoadingQueue.GetCount() > 0)
  {
    return true;
  }

  for (plUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
  {
    if (!s_pState->m_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }

  for (plUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    if (!s_pState->m_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
    {
      return true;
    }
  }
  return false;
}

void plResourceManager::OnEngineShutdown()
{
  plResourceManagerEvent e;
  e.m_Type = plResourceManagerEvent::Type::ManagerShuttingDown;

  // in case of a crash inside the event broadcast or ExecuteAllResourceCleanupCallbacks():
  // you might have a resource type added through a dynamic plugin that has already been unloaded,
  // but the event handler is still referenced
  // to fix this, call plResource::CleanupDynamicPluginReferences() on that resource type during engine shutdown (see plStartup)
  s_pState->m_ManagerEvents.Broadcast(e);

  ExecuteAllResourceCleanupCallbacks();

  EngineAboutToShutdown();

  // unload all resources until there are no more that can be unloaded
  FreeAllUnusedResources();
}

void plResourceManager::OnCoreShutdown()
{
  OnEngineShutdown();

  PLASMA_LOG_BLOCK("Referenced Resources");

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    const plRTTI* pRtti = itType.Key();
    LoadedResources& lr = itType.Value();

    if (!lr.m_Resources.IsEmpty())
    {
      PLASMA_LOG_BLOCK("Type", pRtti->GetTypeName());

      plLog::Error("{0} resource of type '{1}' are still referenced.", lr.m_Resources.GetCount(), pRtti->GetTypeName());

      for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
      {
        plResource* pReference = it.Value();

        plLog::Info("RC = {0}, ID = '{1}'", pReference->GetReferenceCount(), plArgSensitive(pReference->GetResourceID(), "ResourceID"));

#if PLASMA_ENABLED(PLASMA_RESOURCEHANDLE_STACK_TRACES)
        pReference->PrintHandleStackTraces();
#endif
      }
    }
  }

  plPlugin::Events().RemoveEventHandler(PluginEventHandler);

  s_pState.Clear();
}

plResource* plResourceManager::GetResource(const plRTTI* pRtti, plStringView sResourceID, bool bIsReloadable)
{
  if (sResourceID.IsEmpty())
    return nullptr;

  PLASMA_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Calling code must lock the mutex until the resource pointer is stored in a handle");

  // redirect requested type to override type, if available
  pRtti = FindResourceTypeOverride(pRtti, sResourceID);

  PLASMA_ASSERT_DEBUG(pRtti != nullptr, "There is no RTTI information available for the given resource type '{0}'", PLASMA_STRINGIZE(ResourceType));
  PLASMA_ASSERT_DEBUG(pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate(), "There is no RTTI allocator available for the given resource type '{0}'", PLASMA_STRINGIZE(ResourceType));

  plResource* pResource = nullptr;
  plTempHashedString sHashedResourceID(sResourceID);

  plHashedString* redirection;
  if (s_pState->m_NamedResources.TryGetValue(sHashedResourceID, redirection))
  {
    sHashedResourceID = *redirection;
    sResourceID = redirection->GetView();
  }

  LoadedResources& lr = s_pState->m_LoadedResources[pRtti];

  if (lr.m_Resources.TryGetValue(sHashedResourceID, pResource))
    return pResource;

  plResource* pNewResource = pRtti->GetAllocator()->Allocate<plResource>();
  pNewResource->m_Priority = s_pState->m_ResourceTypePriorities.GetValueOrDefault(pRtti, plResourcePriority::Medium);
  pNewResource->SetUniqueID(sResourceID, bIsReloadable);
  pNewResource->m_Flags.AddOrRemove(plResourceFlags::ResourceHasTypeFallback, pNewResource->HasResourceTypeLoadingFallback());

  lr.m_Resources.Insert(sHashedResourceID, pNewResource);

  return pNewResource;
}

void plResourceManager::RegisterResourceOverrideType(const plRTTI* pDerivedTypeToUse, plDelegate<bool(const plStringBuilder&)> overrideDecider)
{
  const plRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != plGetStaticRTTI<plResource>())
  {
    auto& info = s_pState->m_DerivedTypeInfos[pParentType].ExpandAndGetRef();
    info.m_pDerivedType = pDerivedTypeToUse;
    info.m_Decider = overrideDecider;

    pParentType = pParentType->GetParentType();
  }
}

void plResourceManager::UnregisterResourceOverrideType(const plRTTI* pDerivedTypeToUse)
{
  const plRTTI* pParentType = pDerivedTypeToUse->GetParentType();
  while (pParentType != nullptr && pParentType != plGetStaticRTTI<plResource>())
  {
    auto it = s_pState->m_DerivedTypeInfos.Find(pParentType);
    pParentType = pParentType->GetParentType();

    if (!it.IsValid())
      break;

    auto& infos = it.Value();

    for (plUInt32 i = infos.GetCount(); i > 0; --i)
    {
      if (infos[i - 1].m_pDerivedType == pDerivedTypeToUse)
        infos.RemoveAtAndSwap(i - 1);
    }
  }
}

const plRTTI* plResourceManager::FindResourceTypeOverride(const plRTTI* pRtti, plStringView sResourceID)
{
  auto it = s_pState->m_DerivedTypeInfos.Find(pRtti);

  if (!it.IsValid())
    return pRtti;

  plStringBuilder sRedirectedPath;
  plFileSystem::ResolveAssetRedirection(sResourceID, sRedirectedPath);

  while (it.IsValid())
  {
    for (const auto& info : it.Value())
    {
      if (info.m_Decider(sRedirectedPath))
      {
        pRtti = info.m_pDerivedType;
        it = s_pState->m_DerivedTypeInfos.Find(pRtti);
        continue;
      }
    }

    break;
  }

  return pRtti;
}

plString plResourceManager::GenerateUniqueResourceID(plStringView sResourceIDPrefix)
{
  plStringBuilder resourceID;
  resourceID.Format("{}-{}", sResourceIDPrefix, s_pState->m_uiNextResourceID++);
  return resourceID;
}

plTypelessResourceHandle plResourceManager::GetExistingResourceByType(const plRTTI* pResourceType, plStringView sResourceID)
{
  plResource* pResource = nullptr;

  const plTempHashedString sResourceHash(sResourceID);

  PLASMA_LOCK(s_ResourceMutex);

  const plRTTI* pRtti = FindResourceTypeOverride(pResourceType, sResourceID);

  if (s_pState->m_LoadedResources[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return plTypelessResourceHandle(pResource);

  return plTypelessResourceHandle();
}

plTypelessResourceHandle plResourceManager::GetExistingResourceOrCreateAsync(const plRTTI* pResourceType, plStringView sResourceID, plUniquePtr<plResourceTypeLoader>&& pLoader)
{
  PLASMA_LOCK(s_ResourceMutex);

  plTypelessResourceHandle hResource = GetExistingResourceByType(pResourceType, sResourceID);

  if (hResource.IsValid())
    return hResource;

  hResource = GetResource(pResourceType, sResourceID, false);
  plResource* pResource = hResource.m_pResource;

  pResource->m_Flags.Add(plResourceFlags::HasCustomDataLoader | plResourceFlags::IsCreatedResource);
  s_pState->m_CustomLoaders[pResource] = std::move(pLoader);

  return hResource;
}

void plResourceManager::ForceLoadResourceNow(const plTypelessResourceHandle& hResource)
{
  PLASMA_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  plResource* pResource = hResource.m_pResource;

  if (pResource->GetLoadingState() != plResourceState::LoadedResourceMissing && pResource->GetLoadingState() != plResourceState::Loaded)
  {
    InternalPreloadResource(pResource, true);

    EnsureResourceLoadingState(hResource.m_pResource, plResourceState::Loaded);
  }
}

void plResourceManager::RegisterNamedResource(plStringView sLookupName, plStringView sRedirectionResource)
{
  PLASMA_LOCK(s_ResourceMutex);

  plTempHashedString lookup(sLookupName);

  plHashedString redirection;
  redirection.Assign(sRedirectionResource);

  s_pState->m_NamedResources[lookup] = redirection;
}

void plResourceManager::UnregisterNamedResource(plStringView sLookupName)
{
  PLASMA_LOCK(s_ResourceMutex);

  plTempHashedString hash(sLookupName);
  s_pState->m_NamedResources.Remove(hash);
}

void plResourceManager::SetResourceLowResData(const plTypelessResourceHandle& hResource, plStreamReader* pStream)
{
  plResource* pResource = hResource.m_pResource;

  if (pResource->GetBaseResourceFlags().IsSet(plResourceFlags::HasLowResData))
    return;

  if (!pResource->GetBaseResourceFlags().IsSet(plResourceFlags::IsReloadable))
    return;

  PLASMA_LOCK(s_ResourceMutex);

  // set this, even if we don't end up using the data (because some thread is already loading the full thing)
  pResource->m_Flags.Add(plResourceFlags::HasLowResData);

  if (IsQueuedForLoading(pResource))
  {
    // if we cannot find it in the queue anymore, some thread already started loading it
    // in this case, do not try to modify it
    if (RemoveFromLoadingQueue(pResource).Failed())
      return;
  }

  pResource->CallUpdateContent(pStream);

  PLASMA_ASSERT_DEV(pResource->GetLoadingState() != plResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    plResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    pResource->UpdateMemoryUsage(MemUsage);

    PLASMA_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", pResource->GetResourceID());
    PLASMA_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", pResource->GetResourceID());

    pResource->m_MemoryUsage = MemUsage;
  }
}

plResourceTypeLoader* plResourceManager::GetDefaultResourceLoader()
{
  return s_pState->m_pDefaultResourceLoader;
}

void plResourceManager::EnableExportMode(bool bEnable)
{
  PLASMA_ASSERT_DEV(s_pState != nullptr, "plStartup::StartupCoreSystems() must be called before using the plResourceManager.");

  s_pState->m_bExportMode = bEnable;
}

bool plResourceManager::IsExportModeEnabled()
{
  PLASMA_ASSERT_DEV(s_pState != nullptr, "plStartup::StartupCoreSystems() must be called before using the plResourceManager.");

  return s_pState->m_bExportMode;
}

void plResourceManager::RestoreResource(const plTypelessResourceHandle& hResource)
{
  PLASMA_ASSERT_DEV(hResource.IsValid(), "Cannot access an invalid resource");

  plResource* pResource = hResource.m_pResource;
  pResource->m_Flags.Remove(plResourceFlags::PreventFileReload);

  ReloadResource(pResource, true);
}

plUInt32 plResourceManager::GetForceNoFallbackAcquisition()
{
  return s_pState->m_uiForceNoFallbackAcquisition;
}

plTime plResourceManager::GetLastFrameUpdate()
{
  return s_pState->m_LastFrameUpdate;
}

plHashTable<const plRTTI*, plResourceManager::LoadedResources>& plResourceManager::GetLoadedResources()
{
  return s_pState->m_LoadedResources;
}

plDynamicArray<plResource*>& plResourceManager::GetLoadedResourceOfTypeTempContainer()
{
  return s_pState->m_LoadedResourceOfTypeTempContainer;
}

void plResourceManager::SetDefaultResourceLoader(plResourceTypeLoader* pDefaultLoader)
{
  PLASMA_LOCK(s_ResourceMutex);

  s_pState->m_pDefaultResourceLoader = pDefaultLoader;
}

plResourceManager::ResourceTypeInfo& plResourceManager::GetResourceTypeInfo(const plRTTI* pRtti)
{
  return s_pState->m_TypeInfo[pRtti];
}

PLASMA_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceManager);