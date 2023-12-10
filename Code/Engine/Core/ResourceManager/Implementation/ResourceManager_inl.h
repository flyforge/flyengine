#pragma once

#include <Foundation/Logging/Log.h>

template <typename ResourceType>
ResourceType* plResourceManager::GetResource(plStringView sResourceID, bool bIsReloadable)
{
  return static_cast<ResourceType*>(GetResource(plGetStaticRTTI<ResourceType>(), sResourceID, bIsReloadable));
}

template <typename ResourceType>
plTypedResourceHandle<ResourceType> plResourceManager::LoadResource(plStringView sResourceID)
{
  // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
  PLASMA_LOCK(s_ResourceMutex);
  return plTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
}

template <typename ResourceType>
plTypedResourceHandle<ResourceType> plResourceManager::LoadResource(plStringView sResourceID, plTypedResourceHandle<ResourceType> hLoadingFallback)
{
  plTypedResourceHandle<ResourceType> hResource;
  {
    // the mutex here is necessary to prevent a race between resource unloading and storing the pointer in the handle
    PLASMA_LOCK(s_ResourceMutex);
    hResource = plTypedResourceHandle<ResourceType>(GetResource<ResourceType>(sResourceID, true));
  }

  if (hLoadingFallback.IsValid())
  {
    hResource.m_pResource->SetLoadingFallbackResource(hLoadingFallback);
  }

  return hResource;
}

template <typename ResourceType>
plTypedResourceHandle<ResourceType> plResourceManager::GetExistingResource(plStringView sResourceID)
{
  plResource* pResource = nullptr;

  const plTempHashedString sResourceHash(sResourceID);

  PLASMA_LOCK(s_ResourceMutex);

  const plRTTI* pRtti = FindResourceTypeOverride(plGetStaticRTTI<ResourceType>(), sResourceID);

  if (GetLoadedResources()[pRtti].m_Resources.TryGetValue(sResourceHash, pResource))
    return plTypedResourceHandle<ResourceType>((ResourceType*)pResource);

  return plTypedResourceHandle<ResourceType>();
}

template <typename ResourceType, typename DescriptorType>
plTypedResourceHandle<ResourceType> plResourceManager::CreateResource(plStringView sResourceID, DescriptorType&& descriptor, plStringView sResourceDescription)
{
  static_assert(std::is_rvalue_reference<DescriptorType&&>::value, "Please std::move the descriptor into this function");

  PLASMA_LOG_BLOCK("plResourceManager::CreateResource", sResourceID);

  PLASMA_LOCK(s_ResourceMutex);

  plTypedResourceHandle<ResourceType> hResource(GetResource<ResourceType>(sResourceID, false));

  ResourceType* pResource = BeginAcquireResource(hResource, plResourceAcquireMode::PointerOnly);
  pResource->SetResourceDescription(sResourceDescription);
  pResource->m_Flags.Add(plResourceFlags::IsCreatedResource);

  PLASMA_ASSERT_DEV(pResource->GetLoadingState() == plResourceState::Unloaded, "CreateResource was called on a resource that is already created");

  // If this does not compile, you either passed in the wrong descriptor type for the given resource type
  // or you forgot to std::move the descriptor when calling CreateResource
  {
    auto localDescriptor = std::move(descriptor);
    plResourceLoadDesc ld = pResource->CreateResource(std::move(localDescriptor));
    pResource->VerifyAfterCreateResource(ld);
  }

  PLASMA_ASSERT_DEV(pResource->GetLoadingState() != plResourceState::Unloaded, "CreateResource did not set the loading state properly.");

  EndAcquireResource(pResource);

  return hResource;
}

template <typename ResourceType, typename DescriptorType>
plTypedResourceHandle<ResourceType>
plResourceManager::GetOrCreateResource(plStringView sResourceID, DescriptorType&& descriptor, plStringView sResourceDescription)
{
  PLASMA_LOCK(s_ResourceMutex);
  plTypedResourceHandle<ResourceType> hResource = GetExistingResource<ResourceType>(sResourceID);
  if (!hResource.IsValid())
  {
    hResource = CreateResource<ResourceType, DescriptorType>(sResourceID, std::move(descriptor), sResourceDescription);
  }

  return hResource;
}

PLASMA_FORCE_INLINE plResource* plResourceManager::BeginAcquireResourcePointer(const plRTTI* pType, const plTypelessResourceHandle& hResource)
{
  PLASMA_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

  plResource* pResource = (plResource*)hResource.m_pResource;

  PLASMA_ASSERT_DEBUG(pResource->GetDynamicRTTI()->IsDerivedFrom(pType),
    "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(),
    pType->GetTypeName());

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
ResourceType* plResourceManager::BeginAcquireResource(const plTypedResourceHandle<ResourceType>& hResource, plResourceAcquireMode mode,
  const plTypedResourceHandle<ResourceType>& hFallbackResource, plResourceAcquireResult* out_pAcquireResult /*= nullptr*/)
{
  PLASMA_ASSERT_DEV(hResource.IsValid(), "Cannot acquire a resource through an invalid handle!");

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  const plResource* pCurrentlyUpdatingContent = plResource::GetCurrentlyUpdatingContent();
  if (pCurrentlyUpdatingContent != nullptr)
  {
    PLASMA_LOCK(s_ResourceMutex);
    PLASMA_ASSERT_DEV(mode == plResourceAcquireMode::PointerOnly || IsResourceTypeAcquireDuringUpdateContentAllowed(pCurrentlyUpdatingContent->GetDynamicRTTI(), plGetStaticRTTI<ResourceType>()),
      "Trying to acquire a resource of type '{0}' during '{1}::UpdateContent()'. This has to be enabled by calling "
      "plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<{1}, {0}>(); at engine startup, for example in "
      "plGameApplication::Init_SetupDefaultResources().",
      plGetStaticRTTI<ResourceType>()->GetTypeName(), pCurrentlyUpdatingContent->GetDynamicRTTI()->GetTypeName());
  }
#endif

  ResourceType* pResource = (ResourceType*)hResource.m_hTypeless.m_pResource;

  // PLASMA_ASSERT_DEV(pResource->m_iLockCount < 20, "You probably forgot somewhere to call 'EndAcquireResource' in sync with 'BeginAcquireResource'.");
  PLASMA_ASSERT_DEBUG(pResource->GetDynamicRTTI()->template IsDerivedFrom<ResourceType>(),
    "The requested resource does not have the same type ('{0}') as the resource handle ('{1}').", pResource->GetDynamicRTTI()->GetTypeName(),
    plGetStaticRTTI<ResourceType>()->GetTypeName());

  if (mode == plResourceAcquireMode::AllowLoadingFallback && GetForceNoFallbackAcquisition() > 0)
  {
    mode = plResourceAcquireMode::BlockTillLoaded;
  }

  if (mode == plResourceAcquireMode::PointerOnly)
  {
    if (out_pAcquireResult)
      *out_pAcquireResult = plResourceAcquireResult::Final;

    // pResource->m_iLockCount.Increment();
    return pResource;
  }

  // only set the last accessed time stamp, if it is actually needed, pointer-only access might not mean that the resource is used
  // productively
  pResource->m_LastAcquire = GetLastFrameUpdate();

  if (pResource->GetLoadingState() != plResourceState::LoadedResourceMissing)
  {
    if (pResource->GetLoadingState() != plResourceState::Loaded)
    {
      // if BlockTillLoaded is specified, it will prepended to the preload array, thus will be loaded immediately
      InternalPreloadResource(pResource, mode >= plResourceAcquireMode::BlockTillLoaded);

      if (mode == plResourceAcquireMode::AllowLoadingFallback &&
          (pResource->m_hLoadingFallback.IsValid() || hFallbackResource.IsValid() || GetResourceTypeLoadingFallback<ResourceType>().IsValid()))
      {
        // return the fallback resource for now, if there is one
        if (out_pAcquireResult)
          *out_pAcquireResult = plResourceAcquireResult::LoadingFallback;

        // Fallback order is as follows:
        //  1) Prefer any resource specific fallback resource
        //  2) If not available, use the fallback that is given to BeginAcquireResource, as that is at least specific to the situation
        //  3) If nothing else is available, take the fallback for the whole resource type

        if (pResource->m_hLoadingFallback.IsValid())
          return (ResourceType*)BeginAcquireResource(pResource->m_hLoadingFallback, plResourceAcquireMode::BlockTillLoaded);
        else if (hFallbackResource.IsValid())
          return (ResourceType*)BeginAcquireResource(hFallbackResource, plResourceAcquireMode::BlockTillLoaded);
        else
          return (ResourceType*)BeginAcquireResource(GetResourceTypeLoadingFallback<ResourceType>(), plResourceAcquireMode::BlockTillLoaded);
      }

      EnsureResourceLoadingState(pResource, plResourceState::Loaded);
    }
    else
    {
      // as long as there are more quality levels available, schedule the resource for more loading
      // accessing IsQueuedForLoading without a lock here is save because InternalPreloadResource() will lock and early out if necessary
      // and accidentally skipping InternalPreloadResource() is no problem
      if (IsQueuedForLoading(pResource) == false && pResource->GetNumQualityLevelsLoadable() > 0)
        InternalPreloadResource(pResource, false);
    }
  }

  if (pResource->GetLoadingState() == plResourceState::LoadedResourceMissing)
  {
    // When you get a crash with a stack overflow in this code path, then the resource to be used as the
    // 'missing resource' replacement might be missing itself.

    if (plResourceManager::GetResourceTypeMissingFallback<ResourceType>().IsValid())
    {
      if (out_pAcquireResult)
        *out_pAcquireResult = plResourceAcquireResult::MissingFallback;

      return (ResourceType*)BeginAcquireResource(
        plResourceManager::GetResourceTypeMissingFallback<ResourceType>(), plResourceAcquireMode::BlockTillLoaded);
    }

    if (mode != plResourceAcquireMode::AllowLoadingFallback_NeverFail && mode != plResourceAcquireMode::BlockTillLoaded_NeverFail)
    {
      PLASMA_REPORT_FAILURE("The resource '{0}' of type '{1}' is missing and no fallback is available", pResource->GetResourceID(),
        plGetStaticRTTI<ResourceType>()->GetTypeName());
    }

    if (out_pAcquireResult)
      *out_pAcquireResult = plResourceAcquireResult::None;

    return nullptr;
  }

  if (out_pAcquireResult)
    *out_pAcquireResult = plResourceAcquireResult::Final;

  // pResource->m_iLockCount.Increment();
  return pResource;
}

template <typename ResourceType>
void plResourceManager::EndAcquireResource(ResourceType* pResource)
{
  // PLASMA_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (plInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

PLASMA_FORCE_INLINE void plResourceManager::EndAcquireResourcePointer(plResource* pResource)
{
  // PLASMA_ASSERT_DEV(pResource->m_iLockCount > 0, "The resource lock counter is incorrect: {0}", (plInt32)pResource->m_iLockCount);
  // pResource->m_iLockCount.Decrement();
}

template <typename ResourceType>
plLockedObject<plMutex, plDynamicArray<plResource*>> plResourceManager::GetAllResourcesOfType()
{
  const plRTTI* pBaseType = plGetStaticRTTI<ResourceType>();

  auto& container = GetLoadedResourceOfTypeTempContainer();

  // We use a static container here to ensure its life-time is extended beyond
  // calls to this function as the locked object does not own the passed-in object
  // and thus does not extend the data life-time. It is safe to do this, as the
  // locked object holding the container ensures the container will not be
  // accessed concurrently.
  plLockedObject<plMutex, plDynamicArray<plResource*>> loadedResourcesLock(s_ResourceMutex, &container);

  container.Clear();

  for (auto itType = GetLoadedResources().GetIterator(); itType.IsValid(); itType.Next())
  {
    const plRTTI* pDerivedType = itType.Key();

    if (pDerivedType->IsDerivedFrom(pBaseType))
    {
      const LoadedResources& lr = GetLoadedResources()[pDerivedType];

      container.Reserve(container.GetCount() + lr.m_Resources.GetCount());

      for (auto itResource : lr.m_Resources)
      {
        container.PushBack(itResource.Value());
      }
    }
  }

  return loadedResourcesLock;
}

template <typename ResourceType>
bool plResourceManager::ReloadResource(const plTypedResourceHandle<ResourceType>& hResource, bool bForce)
{
  ResourceType* pResource = BeginAcquireResource(hResource, plResourceAcquireMode::PointerOnly);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResource(pResource);

  return res;
}

PLASMA_FORCE_INLINE bool plResourceManager::ReloadResource(const plRTTI* pType, const plTypelessResourceHandle& hResource, bool bForce)
{
  plResource* pResource = BeginAcquireResourcePointer(pType, hResource);

  bool res = ReloadResource(pResource, bForce);

  EndAcquireResourcePointer(pResource);

  return res;
}

template <typename ResourceType>
plUInt32 plResourceManager::ReloadResourcesOfType(bool bForce)
{
  return ReloadResourcesOfType(plGetStaticRTTI<ResourceType>(), bForce);
}

template <typename ResourceType>
void plResourceManager::SetResourceTypeLoader(plResourceTypeLoader* pCreator)
{
  PLASMA_LOCK(s_ResourceMutex);

  GetResourceTypeLoaders()[plGetStaticRTTI<ResourceType>()] = pCreator;
}

template <typename ResourceType>
plTypedResourceHandle<ResourceType> plResourceManager::GetResourceHandleForExport(plStringView sResourceID)
{
  PLASMA_ASSERT_DEV(IsExportModeEnabled(), "Export mode needs to be enabled");

  return LoadResource<ResourceType>(sResourceID);
}

template <typename ResourceType>
void plResourceManager::SetIncrementalUnloadForResourceType(bool bActive)
{
  GetResourceTypeInfo(plGetStaticRTTI<ResourceType>()).m_bIncrementalUnload = bActive;
}
