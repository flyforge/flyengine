
namespace plInternal
{
  // static
  PL_ALWAYS_INLINE WorldData::HierarchyType::Enum WorldData::GetHierarchyType(bool bIsDynamic)
  {
    return bIsDynamic ? HierarchyType::Dynamic : HierarchyType::Static;
  }

  // static
  template <typename VISITOR>
  PL_FORCE_INLINE plVisitorExecution::Enum WorldData::TraverseHierarchyLevel(Hierarchy::DataBlockArray& blocks, void* pUserData /* = nullptr*/)
  {
    for (WorldData::Hierarchy::DataBlock& block : blocks)
    {
      plGameObject::TransformationData* pCurrentData = block.m_pData;
      plGameObject::TransformationData* pEndData = block.m_pData + block.m_uiCount;

      while (pCurrentData < pEndData)
      {
        plVisitorExecution::Enum execution = VISITOR::Visit(pCurrentData, pUserData);
        if (execution != plVisitorExecution::Continue)
          return execution;

        ++pCurrentData;
      }
    }

    return plVisitorExecution::Continue;
  }

  // static
  template <typename VISITOR>
  PL_FORCE_INLINE plVisitorExecution::Enum WorldData::TraverseHierarchyLevelMultiThreaded(
    Hierarchy::DataBlockArray& blocks, void* pUserData /* = nullptr*/)
  {
    plParallelForParams parallelForParams;
    parallelForParams.m_uiBinSize = 100;
    parallelForParams.m_uiMaxTasksPerThread = 2;
    parallelForParams.m_pTaskAllocator = m_StackAllocator.GetCurrentAllocator();

    plTaskSystem::ParallelFor(
      blocks.GetArrayPtr(),
      [pUserData](plArrayPtr<WorldData::Hierarchy::DataBlock> blocksSlice) {
        for (WorldData::Hierarchy::DataBlock& block : blocksSlice)
        {
          plGameObject::TransformationData* pCurrentData = block.m_pData;
          plGameObject::TransformationData* pEndData = block.m_pData + block.m_uiCount;

          while (pCurrentData < pEndData)
          {
            VISITOR::Visit(pCurrentData, pUserData);
            ++pCurrentData;
          }
        }
      },
      "World DataBlock Traversal Task", parallelForParams);

    return plVisitorExecution::Continue;
  }

  // static
  PL_FORCE_INLINE void WorldData::UpdateGlobalTransform(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter)
  {
    pData->UpdateGlobalTransformWithoutParent(uiUpdateCounter);
    pData->UpdateGlobalBounds();
  }

  // static
  PL_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParent(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter)
  {
    pData->UpdateGlobalTransformWithParent(uiUpdateCounter);
    pData->UpdateGlobalBounds();
  }

  // static
  PL_FORCE_INLINE void WorldData::UpdateGlobalTransformAndSpatialData(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter, plSpatialSystem& spatialSystem)
  {
    pData->UpdateGlobalTransformWithoutParent(uiUpdateCounter);
    pData->UpdateGlobalBoundsAndSpatialData(spatialSystem);
  }

  // static
  PL_FORCE_INLINE void WorldData::UpdateGlobalTransformWithParentAndSpatialData(plGameObject::TransformationData* pData, plUInt32 uiUpdateCounter, plSpatialSystem& spatialSystem)
  {
    pData->UpdateGlobalTransformWithParent(uiUpdateCounter);
    pData->UpdateGlobalBoundsAndSpatialData(spatialSystem);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  PL_ALWAYS_INLINE const plGameObject& WorldData::ConstObjectIterator::operator*() const
  {
    return *m_Iterator;
  }

  PL_ALWAYS_INLINE const plGameObject* WorldData::ConstObjectIterator::operator->() const
  {
    return m_Iterator;
  }

  PL_ALWAYS_INLINE WorldData::ConstObjectIterator::operator const plGameObject*() const
  {
    return m_Iterator;
  }

  PL_ALWAYS_INLINE void WorldData::ConstObjectIterator::Next()
  {
    m_Iterator.Next();

    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  PL_ALWAYS_INLINE bool WorldData::ConstObjectIterator::IsValid() const
  {
    return m_Iterator.IsValid();
  }

  PL_ALWAYS_INLINE void WorldData::ConstObjectIterator::operator++()
  {
    Next();
  }

  PL_ALWAYS_INLINE WorldData::ConstObjectIterator::ConstObjectIterator(ObjectStorage::ConstIterator iterator)
    : m_Iterator(iterator)
  {
    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  PL_ALWAYS_INLINE plGameObject& WorldData::ObjectIterator::operator*()
  {
    return *m_Iterator;
  }

  PL_ALWAYS_INLINE plGameObject* WorldData::ObjectIterator::operator->()
  {
    return m_Iterator;
  }

  PL_ALWAYS_INLINE WorldData::ObjectIterator::operator plGameObject*()
  {
    return m_Iterator;
  }

  PL_ALWAYS_INLINE void WorldData::ObjectIterator::Next()
  {
    m_Iterator.Next();

    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  PL_ALWAYS_INLINE bool WorldData::ObjectIterator::IsValid() const
  {
    return m_Iterator.IsValid();
  }

  PL_ALWAYS_INLINE void WorldData::ObjectIterator::operator++()
  {
    Next();
  }

  PL_ALWAYS_INLINE WorldData::ObjectIterator::ObjectIterator(ObjectStorage::Iterator iterator)
    : m_Iterator(iterator)
  {
    while (m_Iterator.IsValid() && m_Iterator->GetHandle().IsInvalidated())
    {
      m_Iterator.Next();
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  PL_FORCE_INLINE WorldData::InitBatch::InitBatch(plAllocator* pAllocator, plStringView sName, bool bMustFinishWithinOneFrame)
    : m_bMustFinishWithinOneFrame(bMustFinishWithinOneFrame)
    , m_ComponentsToInitialize(pAllocator)
    , m_ComponentsToStartSimulation(pAllocator)
  {
    m_sName.Assign(sName);
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  PL_FORCE_INLINE void WorldData::RegisteredUpdateFunction::FillFromDesc(const plWorldModule::UpdateFunctionDesc& desc)
  {
    m_Function = desc.m_Function;
    m_sFunctionName = desc.m_sFunctionName;
    m_fPriority = desc.m_fPriority;
    m_uiGranularity = desc.m_uiGranularity;
    m_bOnlyUpdateWhenSimulating = desc.m_bOnlyUpdateWhenSimulating;
  }

  PL_FORCE_INLINE bool WorldData::RegisteredUpdateFunction::operator<(const RegisteredUpdateFunction& other) const
  {
    // higher priority comes first
    if (m_fPriority != other.m_fPriority)
      return m_fPriority > other.m_fPriority;

    // sort by function name to ensure determinism
    plInt32 iNameComp = plStringUtils::Compare(m_sFunctionName, other.m_sFunctionName);
    PL_ASSERT_DEV(iNameComp != 0, "An update function with the same name and same priority is already registered. This breaks determinism.");
    return iNameComp < 0;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  PL_ALWAYS_INLINE WorldData::ReadMarker::ReadMarker(const WorldData& data)
    : m_Data(data)
  {
  }

  PL_FORCE_INLINE void WorldData::ReadMarker::Lock()
  {
    PL_ASSERT_DEV(m_Data.m_WriteThreadID == (plThreadID)0 || m_Data.m_WriteThreadID == plThreadUtils::GetCurrentThreadID(),
      "World '{0}' cannot be marked for reading because it is already marked for writing by another thread.", m_Data.m_sName);
    m_Data.m_iReadCounter.Increment();
  }

  PL_ALWAYS_INLINE void WorldData::ReadMarker::Unlock()
  {
    m_Data.m_iReadCounter.Decrement();
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  PL_ALWAYS_INLINE WorldData::WriteMarker::WriteMarker(WorldData& data)
    : m_Data(data)
  {
  }

  PL_FORCE_INLINE void WorldData::WriteMarker::Lock()
  {
    // already locked by this thread?
    if (m_Data.m_WriteThreadID != plThreadUtils::GetCurrentThreadID())
    {
      PL_ASSERT_DEV(m_Data.m_iReadCounter == 0, "World '{0}' cannot be marked for writing because it is already marked for reading.", m_Data.m_sName);
      PL_ASSERT_DEV(m_Data.m_WriteThreadID == (plThreadID)0,
        "World '{0}' cannot be marked for writing because it is already marked for writing by another thread.", m_Data.m_sName);

      m_Data.m_WriteThreadID = plThreadUtils::GetCurrentThreadID();
      m_Data.m_iReadCounter.Increment(); // allow reading as well
    }

    m_Data.m_iWriteCounter++;
  }

  PL_FORCE_INLINE void WorldData::WriteMarker::Unlock()
  {
    m_Data.m_iWriteCounter--;

    if (m_Data.m_iWriteCounter == 0)
    {
      m_Data.m_iReadCounter.Decrement();
      m_Data.m_WriteThreadID = (plThreadID)0;
    }
  }
} // namespace plInternal
