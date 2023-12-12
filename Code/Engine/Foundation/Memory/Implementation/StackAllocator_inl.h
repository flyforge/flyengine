template <plUInt32 TrackingFlags>
plStackAllocator<TrackingFlags>::plStackAllocator(plStringView sName, plAllocatorBase* pParent)
  : plAllocator<plMemoryPolicies::plStackAllocation, TrackingFlags>(sName, pParent)
  , m_DestructData(pParent)
  , m_PtrToDestructDataIndexTable(pParent)
{
}

template <plUInt32 TrackingFlags>
plStackAllocator<TrackingFlags>::~plStackAllocator()
{
  Reset();
}

template <plUInt32 TrackingFlags>
void* plStackAllocator<TrackingFlags>::Allocate(size_t uiSize, size_t uiAlign, plMemoryUtils::DestructorFunction destructorFunc)
{
  PLASMA_LOCK(m_Mutex);

  void* ptr = plAllocator<plMemoryPolicies::plStackAllocation, TrackingFlags>::Allocate(uiSize, uiAlign, destructorFunc);

  if (destructorFunc != nullptr)
  {
    plUInt32 uiIndex = m_DestructData.GetCount();
    m_PtrToDestructDataIndexTable.Insert(ptr, uiIndex);

    auto& data = m_DestructData.ExpandAndGetRef();
    data.m_Func = destructorFunc;
    data.m_Ptr = ptr;
  }

  return ptr;
}

template <plUInt32 TrackingFlags>
void plStackAllocator<TrackingFlags>::Deallocate(void* pPtr)
{
  PLASMA_LOCK(m_Mutex);

  plUInt32 uiIndex;
  if (m_PtrToDestructDataIndexTable.Remove(pPtr, &uiIndex))
  {
    auto& data = m_DestructData[uiIndex];
    data.m_Func = nullptr;
    data.m_Ptr = nullptr;
  }

  plAllocator<plMemoryPolicies::plStackAllocation, TrackingFlags>::Deallocate(pPtr);
}

PLASMA_MSVC_ANALYSIS_WARNING_PUSH

// Disable warning for incorrect operator (compiler complains about the TrackingFlags bitwise and in the case that flags = None)
// even with the added guard of a check that it can't be 0.
PLASMA_MSVC_ANALYSIS_WARNING_DISABLE(6313)

template <plUInt32 TrackingFlags>
void plStackAllocator<TrackingFlags>::Reset()
{
  PLASMA_LOCK(m_Mutex);

  for (plUInt32 i = m_DestructData.GetCount(); i-- > 0;)
  {
    auto& data = m_DestructData[i];
    if (data.m_Func != nullptr)
      data.m_Func(data.m_Ptr);
  }
  m_DestructData.Clear();
  m_PtrToDestructDataIndexTable.Clear();

  this->m_allocator.Reset();
  if ((TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::RemoveAllAllocations(this->m_Id);
  }
  else if ((TrackingFlags & plMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    plAllocatorBase::Stats stats;
    this->m_allocator.FillStats(stats);

    plMemoryTracker::SetAllocatorStats(this->m_Id, stats);
  }
}
PLASMA_MSVC_ANALYSIS_WARNING_POP
