#include <Core/CorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceManagerState.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Profiling/Profiling.h>

plResourceManagerWorkerDataLoad::plResourceManagerWorkerDataLoad() = default;
plResourceManagerWorkerDataLoad::~plResourceManagerWorkerDataLoad() = default;

void plResourceManagerWorkerDataLoad::Execute()
{
  PL_PROFILE_SCOPE("LoadResourceFromDisk");

  plResource* pResourceToLoad = nullptr;
  plResourceTypeLoader* pLoader = nullptr;
  plUniquePtr<plResourceTypeLoader> pCustomLoader;

  {
    PL_LOCK(plResourceManager::s_ResourceMutex);

    if (plResourceManager::s_pState->m_LoadingQueue.IsEmpty())
    {
      plResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
      return;
    }

    plResourceManager::UpdateLoadingDeadlines();

    auto it = plResourceManager::s_pState->m_LoadingQueue.PeekFront();
    pResourceToLoad = it.m_pResource;
    plResourceManager::s_pState->m_LoadingQueue.PopFront();

    if (pResourceToLoad->m_Flags.IsSet(plResourceFlags::HasCustomDataLoader))
    {
      pCustomLoader = std::move(plResourceManager::s_pState->m_CustomLoaders[pResourceToLoad]);
      pLoader = pCustomLoader.Borrow();
      pResourceToLoad->m_Flags.Remove(plResourceFlags::HasCustomDataLoader);
      pResourceToLoad->m_Flags.Add(plResourceFlags::PreventFileReload);
    }
  }

  if (pLoader == nullptr)
    pLoader = plResourceManager::GetResourceTypeLoader(pResourceToLoad->GetDynamicRTTI());

  if (pLoader == nullptr)
    pLoader = pResourceToLoad->GetDefaultResourceTypeLoader();

  PL_ASSERT_DEV(pLoader != nullptr, "No Loader function available for Resource Type '{0}'", pResourceToLoad->GetDynamicRTTI()->GetTypeName());

  plResourceLoadData LoaderData = pLoader->OpenDataStream(pResourceToLoad);

  // we need this info later to do some work in a lock, all the directly following code is outside the lock
  const bool bResourceIsLoadedOnMainThread = pResourceToLoad->GetBaseResourceFlags().IsAnySet(plResourceFlags::UpdateOnMainThread);

  plSharedPtr<plResourceManagerWorkerUpdateContent> pUpdateContentTask;
  plTaskGroupID* pUpdateContentGroup = nullptr;

  PL_LOCK(plResourceManager::s_ResourceMutex);

  // try to find an update content task that has finished and can be reused
  for (plUInt32 i = 0; i < plResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount(); ++i)
  {
    auto& td = plResourceManager::s_pState->m_WorkerTasksUpdateContent[i];

    if (plTaskSystem::IsTaskGroupFinished(td.m_GroupId))
    {
      pUpdateContentTask = td.m_pTask;
      pUpdateContentGroup = &td.m_GroupId;
      break;
    }
  }

  // if no such task could be found, we must allocate a new one
  if (pUpdateContentTask == nullptr)
  {
    plStringBuilder s;
    s.SetFormat("Resource Content Updater {0}", plResourceManager::s_pState->m_WorkerTasksUpdateContent.GetCount());

    auto& td = plResourceManager::s_pState->m_WorkerTasksUpdateContent.ExpandAndGetRef();
    td.m_pTask = PL_DEFAULT_NEW(plResourceManagerWorkerUpdateContent);
    td.m_pTask->ConfigureTask(s, plTaskNesting::Maybe);

    pUpdateContentTask = td.m_pTask;
    pUpdateContentGroup = &td.m_GroupId;
  }

  // always updated together with pUpdateContentTask
  PL_MSVC_ANALYSIS_ASSUME(pUpdateContentGroup != nullptr);

  // set up the data load task and launch it
  {
    pUpdateContentTask->m_LoaderData = LoaderData;
    pUpdateContentTask->m_pLoader = pLoader;
    pUpdateContentTask->m_pCustomLoader = std::move(pCustomLoader);
    pUpdateContentTask->m_pResourceToLoad = pResourceToLoad;

    // schedule the task to run, either on the main thread or on some other thread
    *pUpdateContentGroup = plTaskSystem::StartSingleTask(
      pUpdateContentTask, bResourceIsLoadedOnMainThread ? plTaskPriority::SomeFrameMainThread : plTaskPriority::LateNextFrame);

    // restart the next loading task (this one is about to finish)
    plResourceManager::s_pState->m_bAllowLaunchDataLoadTask = true;
    plResourceManager::RunWorkerTask(nullptr);

    pCustomLoader.Clear();
  }
}


//////////////////////////////////////////////////////////////////////////

plResourceManagerWorkerUpdateContent::plResourceManagerWorkerUpdateContent() = default;
plResourceManagerWorkerUpdateContent::~plResourceManagerWorkerUpdateContent() = default;

void plResourceManagerWorkerUpdateContent::Execute()
{
  if (!m_LoaderData.m_sResourceDescription.IsEmpty())
    m_pResourceToLoad->SetResourceDescription(m_LoaderData.m_sResourceDescription);

  m_pResourceToLoad->CallUpdateContent(m_LoaderData.m_pDataStream);

  if (m_pResourceToLoad->m_uiQualityLevelsLoadable > 0)
  {
    // if the resource can have more details loaded, put it into the preload queue right away again
    plResourceManager::PreloadResource(m_pResourceToLoad);
  }

  // update the file modification date, if available
  if (m_LoaderData.m_LoadedFileModificationDate.IsValid())
    m_pResourceToLoad->m_LoadedFileModificationTime = m_LoaderData.m_LoadedFileModificationDate;

  PL_ASSERT_DEV(m_pResourceToLoad->GetLoadingState() != plResourceState::Unloaded, "The resource should have changed its loading state.");

  // Update Memory Usage
  {
    plResource::MemoryUsage MemUsage;
    MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
    MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
    m_pResourceToLoad->UpdateMemoryUsage(MemUsage);

    PL_ASSERT_DEV(
      MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its CPU memory usage", m_pResourceToLoad->GetResourceID());
    PL_ASSERT_DEV(
      MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '{0}' did not properly update its GPU memory usage", m_pResourceToLoad->GetResourceID());

    m_pResourceToLoad->m_MemoryUsage = MemUsage;
  }

  m_pLoader->CloseDataStream(m_pResourceToLoad, m_LoaderData);

  {
    PL_LOCK(plResourceManager::s_ResourceMutex);
    PL_ASSERT_DEV(plResourceManager::IsQueuedForLoading(m_pResourceToLoad), "Multi-threaded access detected");
    m_pResourceToLoad->m_Flags.Remove(plResourceFlags::IsQueuedForLoading);
    m_pResourceToLoad->m_LastAcquire = plResourceManager::GetLastFrameUpdate();
  }

  m_pLoader = nullptr;
  m_pResourceToLoad = nullptr;
}


