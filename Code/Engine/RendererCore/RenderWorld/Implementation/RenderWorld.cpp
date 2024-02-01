#include <RendererCore/RendererCorePCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

plCVarBool cvar_RenderingMultithreading("Rendering.Multithreading", true, plCVarFlags::Default, "Enables multi-threaded update and rendering");
plCVarBool cvar_RenderingCachingStaticObjects("Rendering.Caching.StaticObjects", true, plCVarFlags::Default, "Enables render data caching of static objects");

plEvent<plView*, plMutex> plRenderWorld::s_ViewCreatedEvent;
plEvent<plView*, plMutex> plRenderWorld::s_ViewDeletedEvent;

plEvent<void*> plRenderWorld::s_CameraConfigsModifiedEvent;
bool plRenderWorld::s_bModifyingCameraConfigs = false;
plMap<plString, plRenderWorld::CameraConfig> plRenderWorld::s_CameraConfigs;

plEvent<const plRenderWorldExtractionEvent&, plMutex> plRenderWorld::s_ExtractionEvent;
plEvent<const plRenderWorldRenderEvent&, plMutex> plRenderWorld::s_RenderEvent;
plUInt64 plRenderWorld::s_uiFrameCounter;

namespace
{
  static bool s_bInExtract;
  static plThreadID s_RenderingThreadID;

  static plMutex s_ExtractTasksMutex;
  static plDynamicArray<plTaskGroupID> s_ExtractTasks;

  static plMutex s_ViewsMutex;
  static plIdTable<plViewId, plView*> s_Views;

  static plDynamicArray<plViewHandle> s_MainViews;

  static plMutex s_ViewsToRenderMutex;
  static plDynamicArray<plView*> s_ViewsToRender;

  static plDynamicArray<plSharedPtr<plRenderPipeline>> s_FilteredRenderPipelines[2];

  struct PipelineToRebuild
  {
    PL_DECLARE_POD_TYPE();

    plRenderPipeline* m_pPipeline;
    plViewHandle m_hView;
  };

  static plMutex s_PipelinesToRebuildMutex;
  static plDynamicArray<PipelineToRebuild> s_PipelinesToRebuild;

  static plProxyAllocator* s_pCacheAllocator;

  static plMutex s_CachedRenderDataMutex;
  using CachedRenderDataPerComponent = plHybridArray<const plRenderData*, 4>;
  static plHashTable<plComponentHandle, CachedRenderDataPerComponent> s_CachedRenderData;
  static plDynamicArray<const plRenderData*> s_DeletedRenderData;

  enum
  {
    MaxNumNewCacheEntries = 32
  };

  static bool s_bWriteRenderPipelineDgml = false;
  static plConsoleFunction<void()> s_ConFunc_WriteRenderPipelineDgml("WriteRenderPipelineDgml", "()", []() { s_bWriteRenderPipelineDgml = true; });
} // namespace

namespace plInternal
{
  struct RenderDataCache
  {
    RenderDataCache(plAllocator* pAllocator)
      : m_PerObjectCaches(pAllocator)
    {
      for (plUInt32 i = 0; i < MaxNumNewCacheEntries; ++i)
      {
        m_NewEntriesPerComponent.PushBack(NewEntryPerComponent(pAllocator));
      }
    }

    struct PerObjectCache
    {
      PerObjectCache() = default;

      PerObjectCache(plAllocator* pAllocator)
        : m_Entries(pAllocator)
      {
      }

      plHybridArray<RenderDataCacheEntry, 4> m_Entries;
      plUInt16 m_uiVersion = 0;
    };

    plDynamicArray<PerObjectCache> m_PerObjectCaches;

    struct NewEntryPerComponent
    {
      NewEntryPerComponent(plAllocator* pAllocator)
        : m_Cache(pAllocator)
      {
      }

      plGameObjectHandle m_hOwnerObject;
      plComponentHandle m_hOwnerComponent;
      PerObjectCache m_Cache;
    };

    plStaticArray<NewEntryPerComponent, MaxNumNewCacheEntries> m_NewEntriesPerComponent;
    plAtomicInteger32 m_NewEntriesCount;
  };

#if PL_ENABLED(PL_PLATFORM_64BIT)
  PL_CHECK_AT_COMPILETIME(sizeof(RenderDataCacheEntry) == 16);
#endif
} // namespace plInternal

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderWorld)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plRenderWorld::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plRenderWorld::OnEngineShutdown();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plViewHandle plRenderWorld::CreateView(const char* szName, plView*& out_pView)
{
  plView* pView = PL_DEFAULT_NEW(plView);

  {
    PL_LOCK(s_ViewsMutex);
    pView->m_InternalId = s_Views.Insert(pView);
  }

  pView->SetName(szName);
  pView->InitializePins();

  pView->m_pRenderDataCache = PL_NEW(s_pCacheAllocator, plInternal::RenderDataCache, s_pCacheAllocator);

  s_ViewCreatedEvent.Broadcast(pView);

  out_pView = pView;
  return pView->GetHandle();
}

void plRenderWorld::DeleteView(const plViewHandle& hView)
{
  plView* pView = nullptr;

  {
    PL_LOCK(s_ViewsMutex);
    if (!s_Views.Remove(hView, &pView))
      return;
  }

  s_ViewDeletedEvent.Broadcast(pView);

  PL_DELETE(s_pCacheAllocator, pView->m_pRenderDataCache);

  {
    PL_LOCK(s_PipelinesToRebuildMutex);

    for (plUInt32 i = s_PipelinesToRebuild.GetCount(); i-- > 0;)
    {
      if (s_PipelinesToRebuild[i].m_hView == hView)
      {
        s_PipelinesToRebuild.RemoveAtAndCopy(i);
      }
    }
  }

  RemoveMainView(hView);

  PL_DEFAULT_DELETE(pView);
}

bool plRenderWorld::TryGetView(const plViewHandle& hView, plView*& out_pView)
{
  PL_LOCK(s_ViewsMutex);
  return s_Views.TryGetValue(hView, out_pView);
}

plView* plRenderWorld::GetViewByUsageHint(plCameraUsageHint::Enum usageHint, plCameraUsageHint::Enum alternativeUsageHint /*= plCameraUsageHint::None*/, const plWorld* pWorld /*= nullptr*/)
{
  PL_LOCK(s_ViewsMutex);

  plView* pAlternativeView = nullptr;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    plView* pView = it.Value();
    if (pWorld != nullptr && pView->GetWorld() != pWorld)
      continue;

    if (pView->GetCameraUsageHint() == usageHint)
    {
      return pView;
    }
    else if (alternativeUsageHint != plCameraUsageHint::None && pView->GetCameraUsageHint() == alternativeUsageHint)
    {
      pAlternativeView = pView;
    }
  }

  return pAlternativeView;
}

void plRenderWorld::AddMainView(const plViewHandle& hView)
{
  PL_ASSERT_DEV(!s_bInExtract, "Cannot add main view during extraction");

  if (!s_MainViews.Contains(hView))
    s_MainViews.PushBack(hView);
}

void plRenderWorld::RemoveMainView(const plViewHandle& hView)
{
  plUInt32 uiIndex = s_MainViews.IndexOf(hView);
  if (uiIndex != plInvalidIndex)
  {
    PL_ASSERT_DEV(!s_bInExtract, "Cannot remove main view during extraction");
    s_MainViews.RemoveAtAndCopy(uiIndex);
  }
}

void plRenderWorld::ClearMainViews()
{
  PL_ASSERT_DEV(!s_bInExtract, "Cannot clear main views during extraction");

  s_MainViews.Clear();
}

plArrayPtr<plViewHandle> plRenderWorld::GetMainViews()
{
  return s_MainViews;
}

void plRenderWorld::CacheRenderData(const plView& view, const plGameObjectHandle& hOwnerObject, const plComponentHandle& hOwnerComponent, plUInt16 uiComponentVersion, plArrayPtr<plInternal::RenderDataCacheEntry> cacheEntries)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    plUInt32 uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount;
    if (uiNewEntriesCount >= MaxNumNewCacheEntries)
    {
      return;
    }

    uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount.Increment();
    if (uiNewEntriesCount <= MaxNumNewCacheEntries)
    {
      auto& newEntry = view.m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntriesCount - 1];
      newEntry.m_hOwnerObject = hOwnerObject;
      newEntry.m_hOwnerComponent = hOwnerComponent;
      newEntry.m_Cache.m_Entries = cacheEntries;
      newEntry.m_Cache.m_uiVersion = uiComponentVersion;
    }
  }
}

void plRenderWorld::DeleteAllCachedRenderData()
{
  PL_PROFILE_SCOPE("DeleteAllCachedRenderData");

  PL_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  {
    PL_LOCK(s_ViewsMutex);

    for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
    {
      plView* pView = it.Value();
      pView->m_pRenderDataCache->m_PerObjectCaches.Clear();
    }
  }

  {
    PL_LOCK(s_CachedRenderDataMutex);

    for (auto it = s_CachedRenderData.GetIterator(); it.IsValid(); ++it)
    {
      auto& cachedRenderDataPerComponent = it.Value();

      for (auto pCachedRenderData : cachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      cachedRenderDataPerComponent.Clear();
    }
  }
}

void plRenderWorld::DeleteCachedRenderData(const plGameObjectHandle& hOwnerObject, const plComponentHandle& hOwnerComponent)
{
  PL_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(hOwnerObject);

  PL_LOCK(s_CachedRenderDataMutex);

  CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
  if (s_CachedRenderData.TryGetValue(hOwnerComponent, pCachedRenderDataPerComponent))
  {
    for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
    {
      s_DeletedRenderData.PushBack(pCachedRenderData);
    }

    s_CachedRenderData.Remove(hOwnerComponent);
  }
}

void plRenderWorld::ResetRenderDataCache(plView& ref_view)
{
  ref_view.m_pRenderDataCache->m_PerObjectCaches.Clear();
  ref_view.m_pRenderDataCache->m_NewEntriesCount = 0;

  if (ref_view.GetWorld() != nullptr)
  {
    if (ref_view.GetWorld()->GetObjectDeletionEvent().HasEventHandler(&plRenderWorld::DeleteCachedRenderDataForObject) == false)
    {
      ref_view.GetWorld()->GetObjectDeletionEvent().AddEventHandler(&plRenderWorld::DeleteCachedRenderDataForObject);
    }
  }
}

void plRenderWorld::DeleteCachedRenderDataForObject(const plGameObject* pOwnerObject)
{
  PL_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(pOwnerObject->GetHandle());

  PL_LOCK(s_CachedRenderDataMutex);

  auto components = pOwnerObject->GetComponents();
  for (auto pComponent : components)
  {
    plComponentHandle hComponent = pComponent->GetHandle();

    CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
    if (s_CachedRenderData.TryGetValue(hComponent, pCachedRenderDataPerComponent))
    {
      for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      s_CachedRenderData.Remove(hComponent);
    }
  }
}

void plRenderWorld::DeleteCachedRenderDataForObjectRecursive(const plGameObject* pOwnerObject)
{
  DeleteCachedRenderDataForObject(pOwnerObject);

  for (auto it = pOwnerObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteCachedRenderDataForObjectRecursive(it);
  }
}

plArrayPtr<const plInternal::RenderDataCacheEntry> plRenderWorld::GetCachedRenderData(const plView& view, const plGameObjectHandle& hOwner, plUInt16 uiComponentVersion)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    const auto& perObjectCaches = view.m_pRenderDataCache->m_PerObjectCaches;
    plUInt32 uiCacheIndex = hOwner.GetInternalID().m_InstanceIndex;
    if (uiCacheIndex < perObjectCaches.GetCount())
    {
      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion == uiComponentVersion)
      {
        return perObjectCache.m_Entries;
      }
    }
  }

  return plArrayPtr<const plInternal::RenderDataCacheEntry>();
}

void plRenderWorld::AddViewToRender(const plViewHandle& hView)
{
  plView* pView = nullptr;
  if (!TryGetView(hView, pView))
    return;

  if (!pView->IsValid())
    return;

  {
    PL_LOCK(s_ViewsToRenderMutex);
    PL_ASSERT_DEV(s_bInExtract, "Render views need to be collected during extraction");

    // make sure the view is put at the end of the array, if it is already there, reorder it
    // this ensures that the views that have been referenced by the last other view, get rendered first
    plUInt32 uiIndex = s_ViewsToRender.IndexOf(pView);
    if (uiIndex != plInvalidIndex)
    {
      s_ViewsToRender.RemoveAtAndCopy(uiIndex);
      s_ViewsToRender.PushBack(pView);
      return;
    }

    s_ViewsToRender.PushBack(pView);
  }

  if (cvar_RenderingMultithreading)
  {
    plTaskGroupID extractTaskID = plTaskSystem::StartSingleTask(pView->GetExtractTask(), plTaskPriority::EarlyThisFrame);

    {
      PL_LOCK(s_ExtractTasksMutex);
      s_ExtractTasks.PushBack(extractTaskID);
    }
  }
  else
  {
    pView->ExtractData();
  }
}

void plRenderWorld::ExtractMainViews()
{
  PL_ASSERT_DEV(!s_bInExtract, "ExtractMainViews must not be called from multiple threads.");

  s_bInExtract = true;

  plRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = plRenderWorldExtractionEvent::Type::BeginExtraction;
  extractionEvent.m_uiFrameCounter = s_uiFrameCounter;
  s_ExtractionEvent.Broadcast(extractionEvent);

  if (cvar_RenderingMultithreading)
  {
    s_ExtractTasks.Clear();

    plTaskGroupID extractTaskID = plTaskSystem::CreateTaskGroup(plTaskPriority::EarlyThisFrame);
    s_ExtractTasks.PushBack(extractTaskID);

    {
      PL_LOCK(s_ViewsMutex);

      for (plUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
      {
        plView* pView = nullptr;
        if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
        {
          s_ViewsToRender.PushBack(pView);
          plTaskSystem::AddTaskToGroup(extractTaskID, pView->GetExtractTask());
        }
      }
    }

    plTaskSystem::StartTaskGroup(extractTaskID);

    {
      PL_PROFILE_SCOPE("Wait for Extraction");

      while (true)
      {
        plTaskGroupID taskID;

        {
          PL_LOCK(s_ExtractTasksMutex);
          if (s_ExtractTasks.IsEmpty())
            break;

          taskID = s_ExtractTasks.PeekBack();
          s_ExtractTasks.PopBack();
        }

        plTaskSystem::WaitForGroup(taskID);
      }
    }
  }
  else
  {
    for (plUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
    {
      plView* pView = nullptr;
      if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
      {
        s_ViewsToRender.PushBack(pView);
        pView->ExtractData();
      }
    }
  }

  // filter out duplicates and reverse order so that dependent views are rendered first
  {
    auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForExtraction()];
    filteredRenderPipelines.Clear();

    for (plUInt32 i = s_ViewsToRender.GetCount(); i-- > 0;)
    {
      auto& pRenderPipeline = s_ViewsToRender[i]->m_pRenderPipeline;
      if (!filteredRenderPipelines.Contains(pRenderPipeline))
      {
        filteredRenderPipelines.PushBack(pRenderPipeline);
      }
    }

    s_ViewsToRender.Clear();
  }

  extractionEvent.m_Type = plRenderWorldExtractionEvent::Type::EndExtraction;
  s_ExtractionEvent.Broadcast(extractionEvent);

  s_bInExtract = false;
}

void plRenderWorld::Render(plRenderContext* pRenderContext)
{
  PL_PROFILE_SCOPE("plRenderWorld::Render");

  plRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type = plRenderWorldRenderEvent::Type::BeginRender;
  renderEvent.m_uiFrameCounter = s_uiFrameCounter;
  {
    PL_PROFILE_SCOPE("BeginRender");
    s_RenderEvent.Broadcast(renderEvent);
  }

  if (!cvar_RenderingMultithreading)
  {
    RebuildPipelines();
  }

  auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForRendering()];

  if (s_bWriteRenderPipelineDgml)
  {
    // Executed via WriteRenderPipelineDgml console command.
    s_bWriteRenderPipelineDgml = false;
    const plDateTime dt = plDateTime::MakeFromTimestamp(plTimestamp::CurrentTimestamp());
    for (plUInt32 i = 0; i < filteredRenderPipelines.GetCount(); ++i)
    {
      auto& pRenderPipeline = filteredRenderPipelines[i];
      plStringBuilder sPath(":appdata/Profiling/", plApplication::GetApplicationInstance()->GetApplicationName());
      sPath.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}_Pipeline{}_{}.dgml", dt.GetYear(), plArgU(dt.GetMonth(), 2, true), plArgU(dt.GetDay(), 2, true), plArgU(dt.GetHour(), 2, true), plArgU(dt.GetMinute(), 2, true), plArgU(dt.GetSecond(), 2, true), i, pRenderPipeline->GetViewName().GetData());

      plDGMLGraph graph(plDGMLGraph::Direction::TopToBottom);
      pRenderPipeline->CreateDgmlGraph(graph);
      if (plDGMLGraphWriter::WriteGraphToFile(sPath, graph).Failed())
      {
        plLog::Error("Failed to write render pipeline dgml: {}", sPath);
      }
    }
  }

  for (auto& pRenderPipeline : filteredRenderPipelines)
  {
    // If we are the only one holding a reference to the pipeline skip rendering. The pipeline is not needed anymore and will be deleted soon.
    if (pRenderPipeline->GetRefCount() > 1)
    {
      pRenderPipeline->Render(pRenderContext);
    }
    pRenderPipeline = nullptr;
  }

  filteredRenderPipelines.Clear();

  renderEvent.m_Type = plRenderWorldRenderEvent::Type::EndRender;
  PL_PROFILE_SCOPE("EndRender");
  s_RenderEvent.Broadcast(renderEvent);
}

void plRenderWorld::BeginFrame()
{
  PL_PROFILE_SCOPE("BeginFrame");

  s_RenderingThreadID = plThreadUtils::GetCurrentThreadID();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    plView* pView = it.Value();
    pView->EnsureUpToDate();
  }

  RebuildPipelines();
}

void plRenderWorld::EndFrame()
{
  PL_PROFILE_SCOPE("EndFrame");

  ++s_uiFrameCounter;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    plView* pView = it.Value();
    if (pView->IsValid())
    {
      pView->ReadBackPassProperties();
    }
  }

  ClearRenderDataCache();
  UpdateRenderDataCache();

  s_RenderingThreadID = (plThreadID)0;
}

bool plRenderWorld::GetUseMultithreadedRendering()
{
  return cvar_RenderingMultithreading;
}


bool plRenderWorld::IsRenderingThread()
{
  return s_RenderingThreadID == plThreadUtils::GetCurrentThreadID();
}

void plRenderWorld::DeleteCachedRenderDataInternal(const plGameObjectHandle& hOwnerObject)
{
  plUInt32 uiCacheIndex = hOwnerObject.GetInternalID().m_InstanceIndex;
  plWorld* pWorld = plWorld::GetWorld(hOwnerObject);

  PL_LOCK(s_ViewsMutex);

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    plView* pView = it.Value();
    if (pView->GetWorld() != nullptr && pView->GetWorld() == pWorld)
    {
      auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

      if (uiCacheIndex < perObjectCaches.GetCount())
      {
        perObjectCaches[uiCacheIndex].m_Entries.Clear();
        perObjectCaches[uiCacheIndex].m_uiVersion = 0;
      }
    }
  }
}

void plRenderWorld::ClearRenderDataCache()
{
  PL_PROFILE_SCOPE("Clear Render Data Cache");

  for (auto pRenderData : s_DeletedRenderData)
  {
    plRenderData* ptr = const_cast<plRenderData*>(pRenderData);
    PL_DELETE(s_pCacheAllocator, ptr);
  }

  s_DeletedRenderData.Clear();
}

void plRenderWorld::UpdateRenderDataCache()
{
  PL_PROFILE_SCOPE("Update Render Data Cache");

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    plView* pView = it.Value();
    plUInt32 uiNumNewEntries = plMath::Min<plInt32>(pView->m_pRenderDataCache->m_NewEntriesCount, MaxNumNewCacheEntries);
    pView->m_pRenderDataCache->m_NewEntriesCount = 0;

    auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

    for (plUInt32 uiNewEntryIndex = 0; uiNewEntryIndex < uiNumNewEntries; ++uiNewEntryIndex)
    {
      auto& newEntries = pView->m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntryIndex];
      PL_ASSERT_DEV(!newEntries.m_hOwnerObject.IsInvalidated(), "Implementation error");

      // find or create cached render data
      auto& cachedRenderDataPerComponent = s_CachedRenderData[newEntries.m_hOwnerComponent];

      const plUInt32 uiNumCachedRenderData = cachedRenderDataPerComponent.GetCount();
      if (uiNumCachedRenderData == 0) // Nothing cached yet
      {
        cachedRenderDataPerComponent = CachedRenderDataPerComponent(s_pCacheAllocator);
      }

      plUInt32 uiCachedRenderDataIndex = 0;
      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (newEntry.m_pRenderData != nullptr)
        {
          if (uiCachedRenderDataIndex >= cachedRenderDataPerComponent.GetCount())
          {
            const plRTTI* pRtti = newEntry.m_pRenderData->GetDynamicRTTI();
            newEntry.m_pRenderData = pRtti->GetAllocator()->Clone<plRenderData>(newEntry.m_pRenderData, s_pCacheAllocator);

            cachedRenderDataPerComponent.PushBack(newEntry.m_pRenderData);
          }
          else
          {
            // replace with cached render data
            newEntry.m_pRenderData = cachedRenderDataPerComponent[uiCachedRenderDataIndex];
          }

          ++uiCachedRenderDataIndex;
        }
      }

      // add entry for this view
      const plUInt32 uiCacheIndex = newEntries.m_hOwnerObject.GetInternalID().m_InstanceIndex;
      perObjectCaches.EnsureCount(uiCacheIndex + 1);

      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion != newEntries.m_Cache.m_uiVersion)
      {
        perObjectCache.m_Entries.Clear();
        perObjectCache.m_uiVersion = newEntries.m_Cache.m_uiVersion;
      }

      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (!perObjectCache.m_Entries.Contains(newEntry))
        {
          perObjectCache.m_Entries.PushBack(newEntry);
        }
      }

      // keep entries sorted, otherwise the logic plExtractor::ExtractRenderData doesn't work
      perObjectCache.m_Entries.Sort();
    }
  }
}

// static
void plRenderWorld::AddRenderPipelineToRebuild(plRenderPipeline* pRenderPipeline, const plViewHandle& hView)
{
  PL_LOCK(s_PipelinesToRebuildMutex);

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    if (pipelineToRebuild.m_hView == hView)
    {
      pipelineToRebuild.m_pPipeline = pRenderPipeline;
      return;
    }
  }

  auto& pipelineToRebuild = s_PipelinesToRebuild.ExpandAndGetRef();
  pipelineToRebuild.m_pPipeline = pRenderPipeline;
  pipelineToRebuild.m_hView = hView;
}

// static
void plRenderWorld::RebuildPipelines()
{
  PL_PROFILE_SCOPE("RebuildPipelines");

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    plView* pView = nullptr;
    if (s_Views.TryGetValue(pipelineToRebuild.m_hView, pView))
    {
      if (pipelineToRebuild.m_pPipeline->Rebuild(*pView) == plRenderPipeline::PipelineState::RebuildError)
      {
        plLog::Error("Failed to rebuild pipeline '{}' for view '{}'", pipelineToRebuild.m_pPipeline->m_sName, pView->GetName());
      }
    }
  }

  s_PipelinesToRebuild.Clear();
}

void plRenderWorld::OnEngineStartup()
{
  s_pCacheAllocator = PL_DEFAULT_NEW(plProxyAllocator, "Cached Render Data", plFoundation::GetDefaultAllocator());

  s_CachedRenderData = plHashTable<plComponentHandle, CachedRenderDataPerComponent>(s_pCacheAllocator);
}

void plRenderWorld::OnEngineShutdown()
{
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  for (auto it : s_CachedRenderData)
  {
    auto& cachedRenderDataPerComponent = it.Value();
    if (cachedRenderDataPerComponent.IsEmpty() == false)
    {
      PL_REPORT_FAILURE("Leaked cached render data of type '{}'", cachedRenderDataPerComponent[0]->GetDynamicRTTI()->GetTypeName());
    }
  }
#endif

  ClearRenderDataCache();

  PL_DEFAULT_DELETE(s_pCacheAllocator);

  s_FilteredRenderPipelines[0].Clear();
  s_FilteredRenderPipelines[1].Clear();

  ClearMainViews();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    plView* pView = it.Value();
    PL_DEFAULT_DELETE(pView);
  }

  s_Views.Clear();
}

void plRenderWorld::BeginModifyCameraConfigs()
{
  PL_ASSERT_DEBUG(!s_bModifyingCameraConfigs, "Recursive call not allowed.");
  s_bModifyingCameraConfigs = true;
}

void plRenderWorld::EndModifyCameraConfigs()
{
  PL_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call plRenderWorld::BeginModifyCameraConfigs first");
  s_bModifyingCameraConfigs = false;
  s_CameraConfigsModifiedEvent.Broadcast(nullptr);
}

void plRenderWorld::ClearCameraConfigs()
{
  PL_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call plRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs.Clear();
}

void plRenderWorld::SetCameraConfig(const char* szName, const CameraConfig& config)
{
  PL_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call plRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs[szName] = config;
}

const plRenderWorld::CameraConfig* plRenderWorld::FindCameraConfig(const char* szName)
{
  auto it = s_CameraConfigs.Find(szName);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

PL_STATICLINK_FILE(RendererCore, RendererCore_RenderWorld_Implementation_RenderWorld);
