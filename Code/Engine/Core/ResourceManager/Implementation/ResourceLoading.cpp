#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Profiling/Profiling.h>

plTypelessResourceHandle plResourceManager::LoadResourceByType(const plRTTI* pResourceType, plStringView sResourceID)
{
  // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
  PL_LOCK(s_ResourceMutex);
  return plTypelessResourceHandle(GetResource(pResourceType, sResourceID, true));
}

void plResourceManager::InternalPreloadResource(plResource* pResource, bool bHighestPriority)
{
  if (s_pState->m_bShutdown)
    return;

  PL_PROFILE_SCOPE("InternalPreloadResource");

  PL_LOCK(s_ResourceMutex);

  // if there is nothing else that could be loaded, just return right away
  if (pResource->GetLoadingState() == plResourceState::Loaded && pResource->GetNumQualityLevelsLoadable() == 0)
  {
    // due to the threading this can happen for all resource types and is valid
    // PL_ASSERT_DEV(!IsQueuedForLoading(pResource), "Invalid flag on resource type '{0}'",
    // pResource->GetDynamicRTTI()->GetTypeName());
    return;
  }

  PL_ASSERT_DEV(!s_pState->m_bExportMode, "Resources should not be loaded in export mode");

  // if we are already loading this resource, early out
  if (IsQueuedForLoading(pResource))
  {
    // however, if it now has highest priority and is still in the loading queue (so not yet started)
    // move it to the front of the queue
    if (bHighestPriority)
    {
      // if it is not in the queue anymore, it has already been started by some thread
      if (RemoveFromLoadingQueue(pResource).Succeeded())
      {
        AddToLoadingQueue(pResource, bHighestPriority);
      }
    }

    return;
  }
  else
  {
    AddToLoadingQueue(pResource, bHighestPriority);

    if (bHighestPriority && plTaskSystem::GetCurrentThreadWorkerType() == plWorkerThreadType::FileAccess)
    {
      plResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
    }

    RunWorkerTask(pResource);
  }
}

void plResourceManager::SetupWorkerTasks()
{
  if (!s_pState->m_bTaskNamesInitialized)
  {
    s_pState->m_bTaskNamesInitialized = true;
    plStringBuilder s;

    {
      static constexpr plUInt32 InitialDataLoadTasks = 4;

      for (plUInt32 i = 0; i < InitialDataLoadTasks; ++i)
      {
        s.SetFormat("Resource Data Loader {0}", i);
        auto& data = s_pState->m_WorkerTasksDataLoad.ExpandAndGetRef();
        data.m_pTask = PL_DEFAULT_NEW(plResourceManagerWorkerDataLoad);
        data.m_pTask->ConfigureTask(s, plTaskNesting::Maybe);
      }
    }

    {
      static constexpr plUInt32 InitialUpdateContentTasks = 16;

      for (plUInt32 i = 0; i < InitialUpdateContentTasks; ++i)
      {
        s.SetFormat("Resource Content Updater {0}", i);
        auto& data = s_pState->m_WorkerTasksUpdateContent.ExpandAndGetRef();
        data.m_pTask = PL_DEFAULT_NEW(plResourceManagerWorkerUpdateContent);
        data.m_pTask->ConfigureTask(s, plTaskNesting::Maybe);
      }
    }
  }
}

void plResourceManager::RunWorkerTask(plResource* pResource)
{
  if (s_pState->m_bShutdown)
    return;

  PL_ASSERT_DEV(s_ResourceMutex.IsLocked(), "");

  SetupWorkerTasks();

  if (s_pState->m_bAllowLaunchDataLoadTask && !s_pState->m_LoadingQueue.IsEmpty())
  {
    s_pState->m_bAllowLaunchDataLoadTask = false;

    for (plUInt32 i = 0; i < s_pState->m_WorkerTasksDataLoad.GetCount(); ++i)
    {
      if (s_pState->m_WorkerTasksDataLoad[i].m_pTask->IsTaskFinished())
      {
        s_pState->m_WorkerTasksDataLoad[i].m_GroupId =
          plTaskSystem::StartSingleTask(s_pState->m_WorkerTasksDataLoad[i].m_pTask, plTaskPriority::FileAccess);
        return;
      }
    }

    // could not find any unused task -> need to create a new one
    {
      plStringBuilder s;
      s.SetFormat("Resource Data Loader {0}", s_pState->m_WorkerTasksDataLoad.GetCount());
      auto& data = s_pState->m_WorkerTasksDataLoad.ExpandAndGetRef();
      data.m_pTask = PL_DEFAULT_NEW(plResourceManagerWorkerDataLoad);
      data.m_pTask->ConfigureTask(s, plTaskNesting::Maybe);
      data.m_GroupId = plTaskSystem::StartSingleTask(data.m_pTask, plTaskPriority::FileAccess);
    }
  }
}

void plResourceManager::ReverseBubbleSortStep(plDeque<LoadingInfo>& data)
{
  // Yep, it's really bubble sort!
  // This will move the entry with the smallest value to the front and move all other values closer to their correct position,
  // which is exactly what we need for the priority queue.
  // We do this once a frame, which gives us nice iterative sorting, with relatively deterministic performance characteristics.

  PL_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  const plUInt32 uiCount = data.GetCount();

  for (plUInt32 i = uiCount; i > 1; --i)
  {
    const plUInt32 idx2 = i - 1;
    const plUInt32 idx1 = i - 2;

    if (data[idx1].m_fPriority > data[idx2].m_fPriority)
    {
      plMath::Swap(data[idx1], data[idx2]);
    }
  }
}

void plResourceManager::UpdateLoadingDeadlines()
{
  if (s_pState->m_LoadingQueue.IsEmpty())
    return;

  PL_ASSERT_DEBUG(s_ResourceMutex.IsLocked(), "Calling code must acquire s_ResourceMutex");

  PL_PROFILE_SCOPE("UpdateLoadingDeadlines");

  const plUInt32 uiCount = s_pState->m_LoadingQueue.GetCount();
  s_pState->m_uiLastResourcePriorityUpdateIdx = plMath::Min(s_pState->m_uiLastResourcePriorityUpdateIdx, uiCount);

  plUInt32 uiUpdateCount = plMath::Min(50u, uiCount - s_pState->m_uiLastResourcePriorityUpdateIdx);

  if (uiUpdateCount == 0)
  {
    s_pState->m_uiLastResourcePriorityUpdateIdx = 0;
    uiUpdateCount = plMath::Min(50u, uiCount - s_pState->m_uiLastResourcePriorityUpdateIdx);
  }

  if (uiUpdateCount > 0)
  {
    {
      PL_PROFILE_SCOPE("EvalLoadingDeadlines");

      const plTime tNow = plTime::Now();

      for (plUInt32 i = 0; i < uiUpdateCount; ++i)
      {
        auto& element = s_pState->m_LoadingQueue[s_pState->m_uiLastResourcePriorityUpdateIdx];
        element.m_fPriority = element.m_pResource->GetLoadingPriority(tNow);
        ++s_pState->m_uiLastResourcePriorityUpdateIdx;
      }
    }

    {
      PL_PROFILE_SCOPE("SortLoadingDeadlines");
      ReverseBubbleSortStep(s_pState->m_LoadingQueue);
    }
  }
}

void plResourceManager::PreloadResource(plResource* pResource)
{
  InternalPreloadResource(pResource, false);
}

void plResourceManager::PreloadResource(const plTypelessResourceHandle& hResource)
{
  PL_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  plResource* pResource = hResource.m_pResource;
  PreloadResource(pResource);
}

plResourceState plResourceManager::GetLoadingState(const plTypelessResourceHandle& hResource)
{
  if (hResource.m_pResource == nullptr)
    return plResourceState::Invalid;

  return hResource.m_pResource->GetLoadingState();
}

plResult plResourceManager::RemoveFromLoadingQueue(plResource* pResource)
{
  PL_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");

  if (!IsQueuedForLoading(pResource))
    return PL_SUCCESS;

  LoadingInfo li;
  li.m_pResource = pResource;

  if (s_pState->m_LoadingQueue.RemoveAndSwap(li))
  {
    pResource->m_Flags.Remove(plResourceFlags::IsQueuedForLoading);
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plResourceManager::AddToLoadingQueue(plResource* pResource, bool bHighestPriority)
{
  PL_ASSERT_DEV(s_ResourceMutex.IsLocked(), "Resource mutex must be locked");
  PL_ASSERT_DEV(IsQueuedForLoading(pResource) == false, "Resource is already in the loading queue");

  pResource->m_Flags.Add(plResourceFlags::IsQueuedForLoading);

  LoadingInfo li;
  li.m_pResource = pResource;

  if (bHighestPriority)
  {
    pResource->SetPriority(plResourcePriority::Critical);
    li.m_fPriority = 0.0f;
    s_pState->m_LoadingQueue.PushFront(li);
  }
  else
  {
    li.m_fPriority = pResource->GetLoadingPriority(s_pState->m_LastFrameUpdate);
    s_pState->m_LoadingQueue.PushBack(li);
  }
}

bool plResourceManager::ReloadResource(plResource* pResource, bool bForce)
{
  PL_LOCK(s_ResourceMutex);

  if (!pResource->m_Flags.IsAnySet(plResourceFlags::IsReloadable))
    return false;

  if (!bForce && pResource->m_Flags.IsAnySet(plResourceFlags::PreventFileReload))
    return false;

  plResourceTypeLoader* pLoader = plResourceManager::GetResourceTypeLoader(pResource->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResource->GetDefaultResourceTypeLoader();

  if (pLoader == nullptr)
    return false;

  // no need to reload resources that are not loaded so far
  if (pResource->GetLoadingState() == plResourceState::Unloaded)
    return false;

  bool bAllowPreloading = true;

  // if the resource is already in the loading queue we can just keep it there
  if (IsQueuedForLoading(pResource))
  {
    bAllowPreloading = false;

    LoadingInfo li;
    li.m_pResource = pResource;

    if (s_pState->m_LoadingQueue.IndexOf(li) == plInvalidIndex)
    {
      // the resource is marked as 'loading' but it is not in the queue anymore
      // that means some task is already working on loading it
      // therefore we should not touch it (especially unload it), it might end up in an inconsistent state

      plLog::Dev(
        "Resource '{0}' is not being reloaded, because it is currently being loaded", plArgSensitive(pResource->GetResourceID(), "ResourceID"));
      return false;
    }
  }

  // if bForce, skip the outdated check
  if (!bForce)
  {
    if (!pLoader->IsResourceOutdated(pResource))
      return false;

    if (pResource->GetLoadingState() == plResourceState::LoadedResourceMissing)
    {
      plLog::Dev("Resource '{0}' is missing and will be tried to be reloaded ('{1}')", plArgSensitive(pResource->GetResourceID(), "ResourceID"),
        plArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
    else
    {
      plLog::Dev("Resource '{0}' is outdated and will be reloaded ('{1}')", plArgSensitive(pResource->GetResourceID(), "ResourceID"),
        plArgSensitive(pResource->GetResourceDescription(), "ResourceDesc"));
    }
  }

  if (pResource->GetBaseResourceFlags().IsSet(plResourceFlags::UpdateOnMainThread) == false || plThreadUtils::IsMainThread())
  {
    // make sure existing data is purged
    pResource->CallUnloadData(plResource::Unload::AllQualityLevels);

    PL_ASSERT_DEV(pResource->GetLoadingState() <= plResourceState::LoadedResourceMissing, "Resource '{0}' should be in an unloaded state now.",
      pResource->GetResourceID());
  }
  else
  {
    s_pState->m_ResourcesToUnloadOnMainThread.Insert(plTempHashedString(pResource->GetResourceID().GetData()), pResource->GetDynamicRTTI());
  }

  if (bAllowPreloading)
  {
    const plTime tNow = s_pState->m_LastFrameUpdate;

    // resources that have been in use recently will be put into the preload queue immediately
    // everything else will be loaded on demand
    if (pResource->GetLastAcquireTime() >= tNow - plTime::MakeFromSeconds(30.0))
    {
      PreloadResource(pResource);
    }
  }

  return true;
}

plUInt32 plResourceManager::ReloadResourcesOfType(const plRTTI* pType, bool bForce)
{
  PL_LOCK(s_ResourceMutex);
  PL_LOG_BLOCK("plResourceManager::ReloadResourcesOfType", pType->GetTypeName());

  plUInt32 count = 0;

  LoadedResources& lr = s_pState->m_LoadedResources[pType];

  for (auto it = lr.m_Resources.GetIterator(); it.IsValid(); ++it)
  {
    if (ReloadResource(it.Value(), bForce))
      ++count;
  }

  return count;
}

plUInt32 plResourceManager::ReloadAllResources(bool bForce)
{
  PL_PROFILE_SCOPE("ReloadAllResources");

  PL_LOCK(s_ResourceMutex);
  PL_LOG_BLOCK("plResourceManager::ReloadAllResources");

  plUInt32 count = 0;

  for (auto itType = s_pState->m_LoadedResources.GetIterator(); itType.IsValid(); ++itType)
  {
    for (auto it = itType.Value().m_Resources.GetIterator(); it.IsValid(); ++it)
    {
      if (ReloadResource(it.Value(), bForce))
        ++count;
    }
  }

  if (count > 0)
  {
    plResourceManagerEvent e;
    e.m_Type = plResourceManagerEvent::Type::ReloadAllResources;

    s_pState->m_ManagerEvents.Broadcast(e);
  }

  return count;
}

void plResourceManager::UpdateResourceWithCustomLoader(const plTypelessResourceHandle& hResource, plUniquePtr<plResourceTypeLoader>&& pLoader)
{
  PL_LOCK(s_ResourceMutex);

  hResource.m_pResource->m_Flags.Add(plResourceFlags::HasCustomDataLoader);
  s_pState->m_CustomLoaders[hResource.m_pResource] = std::move(pLoader);
  // if there was already a custom loader set, but it got no action yet, it is deleted here and replaced with the newer loader

  ReloadResource(hResource.m_pResource, true);
};

void plResourceManager::EnsureResourceLoadingState(plResource* pResourceToLoad, const plResourceState RequestedState)
{
  const plRTTI* pOwnRtti = pResourceToLoad->GetDynamicRTTI();

  // help loading until the requested resource is available
  while ((plInt32)pResourceToLoad->GetLoadingState() < (plInt32)RequestedState &&
         (pResourceToLoad->GetLoadingState() != plResourceState::LoadedResourceMissing))
  {
    plTaskGroupID tgid;

    {
      PL_LOCK(s_ResourceMutex);

      for (plUInt32 i = 0; i < s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
      {
        const plResource* pQueuedResource = s_pState->m_WorkerTasksUpdateContent[i].m_pTask->m_pResourceToLoad;

        if (pQueuedResource != nullptr && pQueuedResource != pResourceToLoad && !s_pState->m_WorkerTasksUpdateContent[i].m_pTask->IsTaskFinished())
        {
          if (!IsResourceTypeAcquireDuringUpdateContentAllowed(pQueuedResource->GetDynamicRTTI(), pOwnRtti))
          {
            tgid = s_pState->m_WorkerTasksUpdateContent[i].m_GroupId;
            break;
          }
        }
      }
    }

    if (tgid.IsValid())
    {
      plTaskSystem::WaitForGroup(tgid);
    }
    else
    {
      // do not use plThreadUtils::YieldTimeSlice here, otherwise the thread is not tagged as 'blocked' in the TaskSystem
      plTaskSystem::WaitForCondition([=]() -> bool
        { return (plInt32)pResourceToLoad->GetLoadingState() >= (plInt32)RequestedState ||
                 (pResourceToLoad->GetLoadingState() == plResourceState::LoadedResourceMissing); });
    }
  }
}


