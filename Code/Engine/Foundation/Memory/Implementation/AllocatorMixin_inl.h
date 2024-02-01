namespace plInternal
{
  template <typename AllocationPolicy, plAllocatorTrackingMode TrackingMode>
  class plAllocatorImpl : public plAllocator
  {
  public:
    plAllocatorImpl(plStringView sName, plAllocator* pParent);
    ~plAllocatorImpl();

    // plAllocator implementation
    virtual void* Allocate(size_t uiSize, size_t uiAlign, plMemoryUtils::DestructorFunction destructorFunc = nullptr) override;
    virtual void Deallocate(void* pPtr) override;
    virtual size_t AllocatedSize(const void* pPtr) override;
    virtual plAllocatorId GetId() const override;
    virtual Stats GetStats() const override;

    plAllocator* GetParent() const;

  protected:
    AllocationPolicy m_allocator;

    plAllocatorId m_Id;
    plThreadID m_ThreadID;
  };

  template <typename AllocationPolicy, plAllocatorTrackingMode TrackingMode, bool HasReallocate>
  class plAllocatorMixinReallocate : public plAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    plAllocatorMixinReallocate(plStringView sName, plAllocator* pParent);
  };

  template <typename AllocationPolicy, plAllocatorTrackingMode TrackingMode>
  class plAllocatorMixinReallocate<AllocationPolicy, TrackingMode, true> : public plAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    plAllocatorMixinReallocate(plStringView sName, plAllocator* pParent);
    virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign) override;
  };
}; // namespace plInternal

template <typename A, plAllocatorTrackingMode TrackingMode>
PL_FORCE_INLINE plInternal::plAllocatorImpl<A, TrackingMode>::plAllocatorImpl(plStringView sName, plAllocator* pParent /* = nullptr */)
  : m_allocator(pParent)
  , m_ThreadID(plThreadUtils::GetCurrentThreadID())
{
  if constexpr (TrackingMode >= plAllocatorTrackingMode::Basics)
  {
    this->m_Id = plMemoryTracker::RegisterAllocator(sName, TrackingMode, pParent != nullptr ? pParent->GetId() : plAllocatorId());
  }
}

template <typename A, plAllocatorTrackingMode TrackingMode>
plInternal::plAllocatorImpl<A, TrackingMode>::~plAllocatorImpl()
{
  if constexpr (TrackingMode >= plAllocatorTrackingMode::Basics)
  {
    plMemoryTracker::DeregisterAllocator(this->m_Id);
  }
}

template <typename A, plAllocatorTrackingMode TrackingMode>
void* plInternal::plAllocatorImpl<A, TrackingMode>::Allocate(size_t uiSize, size_t uiAlign, plMemoryUtils::DestructorFunction destructorFunc)
{
  // zero size allocations always return nullptr without tracking (since deallocate nullptr is ignored)
  if (uiSize == 0)
    return nullptr;

  PL_ASSERT_DEBUG(plMath::IsPowerOf2((plUInt32)uiAlign), "Alignment must be power of two");

  plTime fAllocationTime = plTime::Now();

  void* ptr = m_allocator.Allocate(uiSize, uiAlign);
  PL_ASSERT_DEV(ptr != nullptr, "Could not allocate {0} bytes. Out of memory?", uiSize);

  if constexpr (TrackingMode >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::AddAllocation(this->m_Id, TrackingMode, ptr, uiSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <typename A, plAllocatorTrackingMode TrackingMode>
void plInternal::plAllocatorImpl<A, TrackingMode>::Deallocate(void* pPtr)
{
  if constexpr (TrackingMode >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  m_allocator.Deallocate(pPtr);
}

template <typename A, plAllocatorTrackingMode TrackingMode>
size_t plInternal::plAllocatorImpl<A, TrackingMode>::AllocatedSize(const void* pPtr)
{
  if constexpr (TrackingMode >= plAllocatorTrackingMode::AllocationStats)
  {
    return plMemoryTracker::GetAllocationInfo(this->m_Id, pPtr).m_uiSize;
  }
  else
  {
    return 0;
  }
}

template <typename A, plAllocatorTrackingMode TrackingMode>
plAllocatorId plInternal::plAllocatorImpl<A, TrackingMode>::GetId() const
{
  return this->m_Id;
}

template <typename A, plAllocatorTrackingMode TrackingMode>
plAllocator::Stats plInternal::plAllocatorImpl<A, TrackingMode>::GetStats() const
{
  if constexpr (TrackingMode >= plAllocatorTrackingMode::Basics)
  {
    return plMemoryTracker::GetAllocatorStats(this->m_Id);
  }
  else
  {
    return Stats();
  }
}

template <typename A, plAllocatorTrackingMode TrackingMode>
PL_ALWAYS_INLINE plAllocator* plInternal::plAllocatorImpl<A, TrackingMode>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, plAllocatorTrackingMode TrackingMode, bool HasReallocate>
plInternal::plAllocatorMixinReallocate<A, TrackingMode, HasReallocate>::plAllocatorMixinReallocate(plStringView sName, plAllocator* pParent)
  : plAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, plAllocatorTrackingMode TrackingMode>
plInternal::plAllocatorMixinReallocate<A, TrackingMode, true>::plAllocatorMixinReallocate(plStringView sName, plAllocator* pParent)
  : plAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, plAllocatorTrackingMode TrackingMode>
void* plInternal::plAllocatorMixinReallocate<A, TrackingMode, true>::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  if constexpr (TrackingMode >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  plTime fAllocationTime = plTime::Now();

  void* pNewMem = this->m_allocator.Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);

  if constexpr (TrackingMode >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::AddAllocation(this->m_Id, TrackingMode, pNewMem, uiNewSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return pNewMem;
}
