#pragma once

#include <Core/CoreInternal.h>
PL_CORE_INTERNAL_HEADER

#include <Core/ResourceManager/ResourceManager.h>

class plResourceManagerState
{
private:
  friend class plResource;
  friend class plResourceManager;
  friend class plResourceManagerWorkerDataLoad;
  friend class plResourceManagerWorkerUpdateContent;
  friend class plResourceHandleReadContext;

  /// \name Events
  ///@{

  plEvent<const plResourceEvent&, plMutex> m_ResourceEvents;
  plEvent<const plResourceManagerEvent&, plMutex> m_ManagerEvents;

  ///@}
  /// \name Resource Fallbacks
  ///@{

  plDynamicArray<plResourceManager::ResourceCleanupCB> m_ResourceCleanupCallbacks;

  ///@}
  /// \name Resource Priorities
  ///@{

  plMap<const plRTTI*, plResourcePriority> m_ResourceTypePriorities;

  ///@}

  struct TaskDataUpdateContent
  {
    plSharedPtr<plResourceManagerWorkerUpdateContent> m_pTask;
    plTaskGroupID m_GroupId;
  };

  struct TaskDataDataLoad
  {
    plSharedPtr<plResourceManagerWorkerDataLoad> m_pTask;
    plTaskGroupID m_GroupId;
  };

  bool m_bTaskNamesInitialized = false;
  bool m_bBroadcastExistsEvent = false;
  plUInt32 m_uiForceNoFallbackAcquisition = 0;

  // resources in this queue are waiting for a task to load them
  plDeque<plResourceManager::LoadingInfo> m_LoadingQueue;

  plHashTable<const plRTTI*, plResourceManager::LoadedResources> m_LoadedResources;

  bool m_bAllowLaunchDataLoadTask = true;
  bool m_bShutdown = false;

  plHybridArray<TaskDataUpdateContent, 24> m_WorkerTasksUpdateContent;
  plHybridArray<TaskDataDataLoad, 8> m_WorkerTasksDataLoad;

  plTime m_LastFrameUpdate;
  plUInt32 m_uiLastResourcePriorityUpdateIdx = 0;

  plDynamicArray<plResource*> m_LoadedResourceOfTypeTempContainer;
  plHashTable<plTempHashedString, const plRTTI*> m_ResourcesToUnloadOnMainThread;

  const plRTTI* m_pFreeUnusedLastType = nullptr;
  plTempHashedString m_sFreeUnusedLastResourceID;

  // Type Loaders

  plMap<const plRTTI*, plResourceTypeLoader*> m_ResourceTypeLoader;
  plResourceLoaderFromFile m_FileResourceLoader;
  plResourceTypeLoader* m_pDefaultResourceLoader = &m_FileResourceLoader;
  plMap<plResource*, plUniquePtr<plResourceTypeLoader>> m_CustomLoaders;


  // Override / derived resources

  plMap<const plRTTI*, plHybridArray<plResourceManager::DerivedTypeInfo, 4>> m_DerivedTypeInfos;


  // Named resources

  plHashTable<plTempHashedString, plHashedString> m_NamedResources;

  // Asset system interaction

  plMap<plString, const plRTTI*> m_AssetToResourceType;


  // Export mode

  bool m_bExportMode = false;
  plUInt32 m_uiNextResourceID = 0;

  // Resource Unloading
  plTime m_AutoFreeUnusedTimeout = plTime::MakeZero();
  plTime m_AutoFreeUnusedThreshold = plTime::MakeZero();

  plMap<const plRTTI*, plResourceManager::ResourceTypeInfo> m_TypeInfo;
};
