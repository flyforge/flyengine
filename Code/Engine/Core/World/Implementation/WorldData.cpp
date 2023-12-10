#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>

namespace plInternal
{
  class DefaultCoordinateSystemProvider : public plCoordinateSystemProvider
  {
  public:
    DefaultCoordinateSystemProvider()
      : plCoordinateSystemProvider(nullptr)
    {
    }

    virtual void GetCoordinateSystem(const plVec3& vGlobalPosition, plCoordinateSystem& out_CoordinateSystem) const override
    {
      out_CoordinateSystem.m_vForwardDir = plVec3(1.0f, 0.0f, 0.0f);
      out_CoordinateSystem.m_vRightDir = plVec3(0.0f, 1.0f, 0.0f);
      out_CoordinateSystem.m_vUpDir = plVec3(0.0f, 0.0f, 1.0f);
    }
  };

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  void WorldData::UpdateTask::Execute()
  {
    plWorldModule::UpdateContext context;
    context.m_uiFirstComponentIndex = m_uiStartIndex;
    context.m_uiComponentCount = m_uiCount;

    m_Function(context);
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////

  WorldData::WorldData(plWorldDesc& desc)
    : m_sName(desc.m_sName)
    , m_Allocator(desc.m_sName, plFoundation::GetDefaultAllocator())
    , m_AllocatorWrapper(&m_Allocator)
    , m_BlockAllocator(desc.m_sName, &m_Allocator)
    , m_StackAllocator(desc.m_sName, plFoundation::GetAlignedAllocator())
    , m_ObjectStorage(&m_BlockAllocator, &m_Allocator)
    , m_MaxInitializationTimePerFrame(desc.m_MaxComponentInitializationTimePerFrame)
    , m_Clock(desc.m_sName)
    , m_WriteThreadID((plThreadID)0)
    , m_iWriteCounter(0)
    , m_bSimulateWorld(true)
    , m_bReportErrorWhenStaticObjectMoves(desc.m_bReportErrorWhenStaticObjectMoves)
    , m_ReadMarker(*this)
    , m_WriteMarker(*this)
    , m_pUserData(nullptr)
  {
    m_AllocatorWrapper.Reset();

    if (desc.m_uiRandomNumberGeneratorSeed == 0)
    {
      m_Random.InitializeFromCurrentTime();
    }
    else
    {
      m_Random.Initialize(desc.m_uiRandomNumberGeneratorSeed);
    }

    // insert dummy entry to save some checks
    m_Objects.Insert(nullptr);

#if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
    PLASMA_CHECK_AT_COMPILETIME(sizeof(plGameObject::TransformationData) == 240);
#else
    PLASMA_CHECK_AT_COMPILETIME(sizeof(plGameObject::TransformationData) == 192);
#endif

    PLASMA_CHECK_AT_COMPILETIME(sizeof(plGameObject) == 128);
    PLASMA_CHECK_AT_COMPILETIME(sizeof(QueuedMsgMetaData) == 16);
    PLASMA_CHECK_AT_COMPILETIME(PLASMA_COMPONENT_TYPE_INDEX_BITS <= sizeof(plWorldModuleTypeId) * 8);

    auto pDefaultInitBatch = PLASMA_NEW(&m_Allocator, InitBatch, &m_Allocator, "Default", true);
    pDefaultInitBatch->m_bIsReady = true;
    m_InitBatches.Insert(pDefaultInitBatch);
    m_pDefaultInitBatch = pDefaultInitBatch;
    m_pCurrentInitBatch = pDefaultInitBatch;

    m_pSpatialSystem = std::move(desc.m_pSpatialSystem);
    m_pCoordinateSystemProvider = desc.m_pCoordinateSystemProvider;

    if (m_pSpatialSystem == nullptr && desc.m_bAutoCreateSpatialSystem)
    {
      m_pSpatialSystem = PLASMA_NEW(plFoundation::GetAlignedAllocator(), plSpatialSystem_RegularGrid);
    }

    if (m_pCoordinateSystemProvider == nullptr)
    {
      m_pCoordinateSystemProvider = PLASMA_NEW(&m_Allocator, DefaultCoordinateSystemProvider);
    }

    if (m_pTimeStepSmoothing == nullptr)
    {
      m_pTimeStepSmoothing = PLASMA_NEW(&m_Allocator, plDefaultTimeStepSmoothing);
    }

    m_Clock.SetTimeStepSmoothing(m_pTimeStepSmoothing.Borrow());
  }

  WorldData::~WorldData() = default;

  void WorldData::Clear()
  {
    // allow reading and writing during destruction
    m_WriteThreadID = plThreadUtils::GetCurrentThreadID();
    m_iReadCounter.Increment();

    // deactivate all objects and components before destroying them
    for (auto it = m_ObjectStorage.GetIterator(); it.IsValid(); it.Next())
    {
      it->SetActiveFlag(false);
    }

    // deinitialize all modules before we invalidate the world. Components can still access the world during deinitialization.
    for (plWorldModule* pModule : m_Modules)
    {
      if (pModule != nullptr)
      {
        pModule->Deinitialize();
      }
    }

    // now delete all modules
    for (plWorldModule* pModule : m_Modules)
    {
      if (pModule != nullptr)
      {
        PLASMA_DELETE(&m_Allocator, pModule);
      }
    }
    m_Modules.Clear();

    // this deletes the plGameObject instances
    m_ObjectStorage.Clear();

    // delete all transformation data
    for (plUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
    {
      Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];

      for (plUInt32 i = hierarchy.m_Data.GetCount(); i-- > 0;)
      {
        Hierarchy::DataBlockArray* blocks = hierarchy.m_Data[i];
        for (plUInt32 j = blocks->GetCount(); j-- > 0;)
        {
          m_BlockAllocator.DeallocateBlock((*blocks)[j]);
        }
        PLASMA_DELETE(&m_Allocator, blocks);
      }

      hierarchy.m_Data.Clear();
    }

    // delete task storage
    m_UpdateTasks.Clear();

    // delete queued messages
    for (plUInt32 i = 0; i < plObjectMsgQueueType::COUNT; ++i)
    {
      {
        MessageQueue& queue = m_MessageQueues[i];

        // The messages in this queue are allocated through a frame allocator and thus mustn't (and don't need to be) deallocated
        queue.Clear();
      }

      {
        MessageQueue& queue = m_TimedMessageQueues[i];
        while (!queue.IsEmpty())
        {
          MessageQueue::Entry& entry = queue.Peek();
          PLASMA_DELETE(&m_Allocator, entry.m_pMessage);

          queue.Dequeue();
        }
      }
    }
  }

  plGameObject::TransformationData* WorldData::CreateTransformationData(bool bDynamic, plUInt32 uiHierarchyLevel)
  {
    Hierarchy& hierarchy = m_Hierarchies[GetHierarchyType(bDynamic)];

    while (uiHierarchyLevel >= hierarchy.m_Data.GetCount())
    {
      hierarchy.m_Data.PushBack(PLASMA_NEW(&m_Allocator, Hierarchy::DataBlockArray, &m_Allocator));
    }

    Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];
    Hierarchy::DataBlock* pBlock = nullptr;

    if (!blocks.IsEmpty())
    {
      pBlock = &blocks.PeekBack();
    }

    if (pBlock == nullptr || pBlock->IsFull())
    {
      blocks.PushBack(m_BlockAllocator.AllocateBlock<plGameObject::TransformationData>());
      pBlock = &blocks.PeekBack();
    }

    return pBlock->ReserveBack();
  }

  void WorldData::DeleteTransformationData(bool bDynamic, plUInt32 uiHierarchyLevel, plGameObject::TransformationData* pData)
  {
    Hierarchy& hierarchy = m_Hierarchies[GetHierarchyType(bDynamic)];
    Hierarchy::DataBlockArray& blocks = *hierarchy.m_Data[uiHierarchyLevel];

    Hierarchy::DataBlock& lastBlock = blocks.PeekBack();
    const plGameObject::TransformationData* pLast = lastBlock.PopBack();

    if (pData != pLast)
    {
      plMemoryUtils::Copy(pData, pLast, 1);
      pData->m_pObject->m_pTransformationData = pData;

      // fix parent transform data for children as well
      auto it = pData->m_pObject->GetChildren();
      while (it.IsValid())
      {
        auto pTransformData = it->m_pTransformationData;
        pTransformData->m_pParentData = pData;
        it.Next();
      }
    }

    if (lastBlock.IsEmpty())
    {
      m_BlockAllocator.DeallocateBlock(lastBlock);
      blocks.PopBack();
    }
  }

  void WorldData::TraverseBreadthFirst(VisitorFunc& func)
  {
    struct Helper
    {
      PLASMA_ALWAYS_INLINE static plVisitorExecution::Enum Visit(plGameObject::TransformationData* pData, void* pUserData) { return (*static_cast<VisitorFunc*>(pUserData))(pData->m_pObject); }
    };

    const plUInt32 uiMaxHierarchyLevel = plMath::Max(m_Hierarchies[HierarchyType::Static].m_Data.GetCount(), m_Hierarchies[HierarchyType::Dynamic].m_Data.GetCount());

    for (plUInt32 uiHierarchyLevel = 0; uiHierarchyLevel < uiMaxHierarchyLevel; ++uiHierarchyLevel)
    {
      for (plUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
      {
        Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
        if (uiHierarchyLevel < hierarchy.m_Data.GetCount())
        {
          plVisitorExecution::Enum execution = TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[uiHierarchyLevel], &func);
          PLASMA_ASSERT_DEV(execution != plVisitorExecution::Skip, "Skip is not supported when using breadth first traversal");
          if (execution == plVisitorExecution::Stop)
            return;
        }
      }
    }
  }

  void WorldData::TraverseDepthFirst(VisitorFunc& func)
  {
    struct Helper
    {
      PLASMA_ALWAYS_INLINE static plVisitorExecution::Enum Visit(plGameObject::TransformationData* pData, void* pUserData) { return WorldData::TraverseObjectDepthFirst(pData->m_pObject, *static_cast<VisitorFunc*>(pUserData)); }
    };

    for (plUInt32 uiHierarchyIndex = 0; uiHierarchyIndex < HierarchyType::COUNT; ++uiHierarchyIndex)
    {
      Hierarchy& hierarchy = m_Hierarchies[uiHierarchyIndex];
      if (!hierarchy.m_Data.IsEmpty())
      {
        if (TraverseHierarchyLevel<Helper>(*hierarchy.m_Data[0], &func) == plVisitorExecution::Stop)
          return;
      }
    }
  }

  // static
  plVisitorExecution::Enum WorldData::TraverseObjectDepthFirst(plGameObject* pObject, VisitorFunc& func)
  {
    plVisitorExecution::Enum execution = func(pObject);
    if (execution == plVisitorExecution::Stop)
      return plVisitorExecution::Stop;

    if (execution != plVisitorExecution::Skip) // skip all children
    {
      for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
      {
        if (TraverseObjectDepthFirst(it, func) == plVisitorExecution::Stop)
          return plVisitorExecution::Stop;
      }
    }

    return plVisitorExecution::Continue;
  }

  void WorldData::UpdateGlobalTransforms()
  {
    struct UserData
    {
      plSpatialSystem* m_pSpatialSystem;
      plUInt32 m_uiUpdateCounter;
    };

    UserData userData;
    userData.m_pSpatialSystem = m_pSpatialSystem.Borrow();
    userData.m_uiUpdateCounter = m_uiUpdateCounter;

    struct RootLevel
    {
      PLASMA_ALWAYS_INLINE static plVisitorExecution::Enum Visit(plGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<const UserData*>(pUserData0);
        WorldData::UpdateGlobalTransform(pData, pUserData->m_uiUpdateCounter);
        return plVisitorExecution::Continue;
      }
    };

    struct WithParent
    {
      PLASMA_ALWAYS_INLINE static plVisitorExecution::Enum Visit(plGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<const UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformWithParent(pData, pUserData->m_uiUpdateCounter);
        return plVisitorExecution::Continue;
      }
    };

    struct RootLevelWithSpatialData
    {
      PLASMA_ALWAYS_INLINE static plVisitorExecution::Enum Visit(plGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformAndSpatialData(pData, pUserData->m_uiUpdateCounter, *pUserData->m_pSpatialSystem);
        return plVisitorExecution::Continue;
      }
    };

    struct WithParentWithSpatialData
    {
      PLASMA_ALWAYS_INLINE static plVisitorExecution::Enum Visit(plGameObject::TransformationData* pData, void* pUserData0)
      {
        auto pUserData = static_cast<UserData*>(pUserData0);
        WorldData::UpdateGlobalTransformWithParentAndSpatialData(pData, pUserData->m_uiUpdateCounter, *pUserData->m_pSpatialSystem);
        return plVisitorExecution::Continue;
      }
    };

    Hierarchy& hierarchy = m_Hierarchies[HierarchyType::Dynamic];
    if (!hierarchy.m_Data.IsEmpty())
    {
      auto dataPtr = hierarchy.m_Data.GetData();

      // If we have no spatial system, we perform multi-threaded update as we do not
      // have to acquire a write lock in the process.
      if (m_pSpatialSystem == nullptr)
      {
        TraverseHierarchyLevelMultiThreaded<RootLevel>(*dataPtr[0], &userData);

        for (plUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
        {
          TraverseHierarchyLevelMultiThreaded<WithParent>(*dataPtr[i], &userData);
        }
      }
      else
      {
        TraverseHierarchyLevel<RootLevelWithSpatialData>(*dataPtr[0], &userData);

        for (plUInt32 i = 1; i < hierarchy.m_Data.GetCount(); ++i)
        {
          TraverseHierarchyLevel<WithParentWithSpatialData>(*dataPtr[i], &userData);
        }
      }
    }
  }

} // namespace plInternal


PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_WorldData);
