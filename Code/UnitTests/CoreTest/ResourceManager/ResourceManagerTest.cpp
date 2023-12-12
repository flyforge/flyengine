#include <CoreTest/CoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(ResourceManager);

namespace
{
  using TestResourceHandle = plTypedResourceHandle<class TestResource>;

  class TestResource : public plResource
  {
    PLASMA_ADD_DYNAMIC_REFLECTION(TestResource, plResource);
    PLASMA_RESOURCE_DECLARE_COMMON_CODE(TestResource);

  public:
    TestResource()
      : plResource(plResource::DoUpdate::OnAnyThread, 1)
    {
    }

  protected:
    virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override
    {
      plResourceLoadDesc ld;
      ld.m_State = plResourceState::Unloaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      return ld;
    }

    virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override
    {
      plResourceLoadDesc ld;
      ld.m_State = plResourceState::Loaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      plStreamReader& s = *Stream;

      plUInt32 uiNumElements = 0;
      s >> uiNumElements;

      if (GetResourceID().StartsWith("NonBlockingLevel1-"))
      {
        m_hNested = plResourceManager::LoadResource<TestResource>("Level0-0");
      }

      if (GetResourceID().StartsWith("BlockingLevel1-"))
      {
        m_hNested = plResourceManager::LoadResource<TestResource>("Level0-0");

        plResourceLock<TestResource> pTestResource(m_hNested, plResourceAcquireMode::BlockTillLoaded_NeverFail);

        PLASMA_ASSERT_ALWAYS(pTestResource.GetAcquireResult() == plResourceAcquireResult::Final, "");
      }

      m_Data.SetCountUninitialized(uiNumElements);

      for (plUInt32 i = 0; i < uiNumElements; ++i)
      {
        s >> m_Data[i];
      }

      return ld;
    }

    virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override
    {
      out_NewMemoryUsage.m_uiMemoryCPU = sizeof(TestResource);
      out_NewMemoryUsage.m_uiMemoryGPU = 0;
    }

  public:
    void Test() { PLASMA_TEST_BOOL(!m_Data.IsEmpty()); }

  private:
    TestResourceHandle m_hNested;
    plDynamicArray<plUInt32> m_Data;
  };

  class TestResourceTypeLoader : public plResourceTypeLoader
  {
  public:
    struct LoadedData
    {
      plDefaultMemoryStreamStorage m_StreamData;
      plMemoryStreamReader m_Reader;
    };

    virtual plResourceLoadData OpenDataStream(const plResource* pResource) override
    {
      LoadedData* pData = PLASMA_DEFAULT_NEW(LoadedData);

      const plUInt32 uiNumElements = 1024 * 10;
      pData->m_StreamData.Reserve(uiNumElements * sizeof(plUInt32) + 1);

      plMemoryStreamWriter writer(&pData->m_StreamData);
      pData->m_Reader.SetStorage(&pData->m_StreamData);

      writer << uiNumElements;

      for (plUInt32 i = 0; i < uiNumElements; ++i)
      {
        writer << i;
      }

      plResourceLoadData ld;
      ld.m_pCustomLoaderData = pData;
      ld.m_pDataStream = &pData->m_Reader;
      ld.m_sResourceDescription = pResource->GetResourceID();

      return ld;
    }

    virtual void CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData) override
    {
      LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);
      PLASMA_DEFAULT_DELETE(pData);
    }
  };

  PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(TestResource);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(TestResource, 1, plRTTIDefaultAllocator<TestResource>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;

} // namespace

PLASMA_CREATE_SIMPLE_TEST(ResourceManager, Basics)
{
  TestResourceTypeLoader TypeLoader;
  plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  plResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  PLASMA_SCOPE_EXIT(plResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Main")
  {
    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const plUInt32 uiNumResources = 200;

    plDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    plStringBuilder sResourceID;
    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.Format("Level0-{}", i);
      hResources.PushBack(plResourceManager::LoadResource<TestResource>(sResourceID));
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      plResourceManager::PreloadResource(hResources[i]);
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      plResourceLock<TestResource> pTestResource(hResources[i], plResourceAcquireMode::BlockTillLoaded_NeverFail);

      PLASMA_TEST_BOOL(pTestResource.GetAcquireResult() == plResourceAcquireResult::Final);

      pTestResource->Test();
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    hResources.Clear();

    plUInt32 uiUnloaded = 0;

    for (plUInt32 tries = 0; tries < 3; ++tries)
    {
      // if a resource is in a loading queue, unloading it can actually 'fail' for a short time
      uiUnloaded += plResourceManager::FreeAllUnusedResources();

      if (uiUnloaded == uiNumResources)
        break;

      plThreadUtils::Sleep(plTime::Milliseconds(100));
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}

PLASMA_CREATE_SIMPLE_TEST(ResourceManager, NestedLoading)
{
  TestResourceTypeLoader TypeLoader;
  plResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  plResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  PLASMA_SCOPE_EXIT(plResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "NonBlocking")
  {
    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const plUInt32 uiNumResources = 200;

    plDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    plStringBuilder sResourceID;
    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.Format("NonBlockingLevel1-{}", i);
      hResources.PushBack(plResourceManager::LoadResource<TestResource>(sResourceID));
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      plResourceManager::PreloadResource(hResources[i]);
    }

    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      plResourceLock<TestResource> pTestResource(hResources[i], plResourceAcquireMode::BlockTillLoaded_NeverFail);

      PLASMA_TEST_BOOL(pTestResource.GetAcquireResult() == plResourceAcquireResult::Final);

      pTestResource->Test();
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    plResourceManager::FreeAllUnusedResources();
    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }

  // Test disabled as it deadlocks
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Blocking")
  {
    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const plUInt32 uiNumResources = 500;

    plDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    plStringBuilder sResourceID;
    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.Format("BlockingLevel1-{}", i);
      hResources.PushBack(plResourceManager::LoadResource<TestResource>(sResourceID));
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      plResourceManager::PreloadResource(hResources[i]);
    }

    for (plUInt32 i = 0; i < uiNumResources; ++i)
    {
      plResourceLock<TestResource> pTestResource(hResources[i], plResourceAcquireMode::BlockTillLoaded_NeverFail);

      PLASMA_TEST_BOOL(pTestResource.GetAcquireResult() == plResourceAcquireResult::Final);

      pTestResource->Test();
    }

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (plResourceManager::IsAnyLoadingInProgress())
    {
      plThreadUtils::Sleep(plTime::Milliseconds(100));
    }

    plResourceManager::FreeAllUnusedResources();
    plThreadUtils::Sleep(plTime::Milliseconds(100));
    plResourceManager::FreeAllUnusedResources();

    PLASMA_TEST_INT(plResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}
