namespace plInternal
{
  template <typename AllocationPolicy, plUInt32 TrackingFlags>
  class plAllocatorImpl : public plAllocatorBase
  {
  public:
    plAllocatorImpl(plStringView sName, plAllocatorBase* pParent);
    ~plAllocatorImpl();

    // plAllocatorBase implementation
    virtual void* Allocate(size_t uiSize, size_t uiAlign, plMemoryUtils::DestructorFunction destructorFunc = nullptr) override;
    virtual void Deallocate(void* pPtr) override;
    virtual size_t AllocatedSize(const void* pPtr) override;
    virtual plAllocatorId GetId() const override;
    virtual Stats GetStats() const override;

    plAllocatorBase* GetParent() const;

  protected:
    AllocationPolicy m_allocator;

    plAllocatorId m_Id;
    plThreadID m_ThreadID;
  };

  template <typename AllocationPolicy, plUInt32 TrackingFlags, bool HasReallocate>
  class plAllocatorMixinReallocate : public plAllocatorImpl<AllocationPolicy, TrackingFlags>
  {
  public:
    plAllocatorMixinReallocate(plStringView sName, plAllocatorBase* pParent);
  };

  template <typename AllocationPolicy, plUInt32 TrackingFlags>
  class plAllocatorMixinReallocate<AllocationPolicy, TrackingFlags, true> : public plAllocatorImpl<AllocationPolicy, TrackingFlags>
  {
  public:
    plAllocatorMixinReallocate(plStringView sName, plAllocatorBase* pParent);
    virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign) override;
  };
}; // namespace plInternal

template <typename A, plUInt32 TrackingFlags>
PLASMA_FORCE_INLINE plInternal::plAllocatorImpl<A, TrackingFlags>::plAllocatorImpl(plStringView sName, plAllocatorBase* pParent /* = nullptr */)
  : m_allocator(pParent)
  , m_ThreadID(plThreadUtils::GetCurrentThreadID())
{
  if constexpr ((TrackingFlags & plMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG((TrackingFlags & ~plMemoryTrackingFlags::All) == 0, "Invalid tracking flags");
    const plUInt32 uiTrackingFlags = TrackingFlags;
    plBitflags<plMemoryTrackingFlags> flags = *reinterpret_cast<const plBitflags<plMemoryTrackingFlags>*>(&uiTrackingFlags);
    this->m_Id = plMemoryTracker::RegisterAllocator(sName, flags, pParent != nullptr ? pParent->GetId() : plAllocatorId());
  }
}

template <typename A, plUInt32 TrackingFlags>
plInternal::plAllocatorImpl<A, TrackingFlags>::~plAllocatorImpl()
{
  // PLASMA_ASSERT_RELEASE(m_ThreadID == plThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");

  if constexpr ((TrackingFlags & plMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    plMemoryTracker::DeregisterAllocator(this->m_Id);
  }
}

template <typename A, plUInt32 TrackingFlags>
void* plInternal::plAllocatorImpl<A, TrackingFlags>::Allocate(size_t uiSize, size_t uiAlign, plMemoryUtils::DestructorFunction destructorFunc)
{
  // zero size allocations always return nullptr without tracking (since deallocate nullptr is ignored)
  if (uiSize == 0)
    return nullptr;

  PLASMA_ASSERT_DEBUG(plMath::IsPowerOf2((plUInt32)uiAlign), "Alignment must be power of two");

  plTime fAllocationTime = plTime::Now();

  void* ptr = m_allocator.Allocate(uiSize, uiAlign);
  PLASMA_ASSERT_DEV(ptr != nullptr, "Could not allocate {0} bytes. Out of memory?", uiSize);

  if constexpr ((TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plBitflags<plMemoryTrackingFlags> flags;
    flags.SetValue(TrackingFlags);

    plMemoryTracker::AddAllocation(this->m_Id, flags, ptr, uiSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <typename A, plUInt32 TrackingFlags>
void plInternal::plAllocatorImpl<A, TrackingFlags>::Deallocate(void* pPtr)
{
  if constexpr ((TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  m_allocator.Deallocate(pPtr);
}

template <typename A, plUInt32 TrackingFlags>
size_t plInternal::plAllocatorImpl<A, TrackingFlags>::AllocatedSize(const void* pPtr)
{
  if constexpr ((TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    return plMemoryTracker::GetAllocationInfo(this->m_Id, pPtr).m_uiSize;
  }
  else
  {
    return 0;
  }
}

template <typename A, plUInt32 TrackingFlags>
plAllocatorId plInternal::plAllocatorImpl<A, TrackingFlags>::GetId() const
{
  return this->m_Id;
}

template <typename A, plUInt32 TrackingFlags>
plAllocatorBase::Stats plInternal::plAllocatorImpl<A, TrackingFlags>::GetStats() const
{
  if constexpr ((TrackingFlags & plMemoryTrackingFlags::RegisterAllocator) != 0)
  {
    return plMemoryTracker::GetAllocatorStats(this->m_Id);
  }
  else
  {
    return Stats();
  }
}

template <typename A, plUInt32 TrackingFlags>
PLASMA_ALWAYS_INLINE plAllocatorBase* plInternal::plAllocatorImpl<A, TrackingFlags>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, plUInt32 TrackingFlags, bool HasReallocate>
plInternal::plAllocatorMixinReallocate<A, TrackingFlags, HasReallocate>::plAllocatorMixinReallocate(plStringView sName, plAllocatorBase* pParent)
  : plAllocatorImpl<A, TrackingFlags>(sName, pParent)
{
}

template <typename A, plUInt32 TrackingFlags>
plInternal::plAllocatorMixinReallocate<A, TrackingFlags, true>::plAllocatorMixinReallocate(plStringView sName, plAllocatorBase* pParent)
  : plAllocatorImpl<A, TrackingFlags>(sName, pParent)
{
}

template <typename A, plUInt32 TrackingFlags>
void* plInternal::plAllocatorMixinReallocate<A, TrackingFlags, true>::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  if constexpr ((TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  plTime fAllocationTime = plTime::Now();

  void* pNewMem = this->m_allocator.Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);

  if constexpr ((TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plBitflags<plMemoryTrackingFlags> flags;
    flags.SetValue(TrackingFlags);

    plMemoryTracker::AddAllocation(this->m_Id, flags, pNewMem, uiNewSize, uiAlign, plTime::Now() - fAllocationTime);
  }
  return pNewMem;
}
