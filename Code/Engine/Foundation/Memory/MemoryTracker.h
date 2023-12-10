#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>

struct plMemoryTrackingFlags
{
  using StorageType = plUInt32;

  enum Enum
  {
    None,
    RegisterAllocator = PLASMA_BIT(0),        ///< Register the allocator with the memory tracker. If EnableAllocationTracking is not set as well it is up to the
                                          ///< allocator implementation whether it collects usable stats or not.
    EnableAllocationTracking = PLASMA_BIT(1), ///< Enable tracking of individual allocations
    EnableStackTrace = PLASMA_BIT(2),         ///< Enable stack traces for each allocation

    All = RegisterAllocator | EnableAllocationTracking | EnableStackTrace,

    Default = 0
#if PLASMA_ENABLED(PLASMA_USE_ALLOCATION_TRACKING)
              | RegisterAllocator | EnableAllocationTracking
#endif
#if PLASMA_ENABLED(PLASMA_USE_ALLOCATION_STACK_TRACING)
              | EnableStackTrace
#endif
  };

  struct Bits
  {
    StorageType RegisterAllocator : 1;
    StorageType EnableAllocationTracking : 1;
    StorageType EnableStackTrace : 1;
  };
};

// PLASMA_DECLARE_FLAGS_OPERATORS(plMemoryTrackingFlags);

#define PLASMA_STATIC_ALLOCATOR_NAME "Statics"

/// \brief Memory tracker which keeps track of all allocations and constructions
class PLASMA_FOUNDATION_DLL plMemoryTracker
{
public:
  struct AllocationInfo
  {
    PLASMA_DECLARE_POD_TYPE();

    PLASMA_FORCE_INLINE AllocationInfo()

      = default;

    void** m_pStackTrace = nullptr;
    size_t m_uiSize = 0;
    plUInt16 m_uiAlignment = 0;
    plUInt16 m_uiStackTraceLength = 0;

    PLASMA_ALWAYS_INLINE const plArrayPtr<void*> GetStackTrace() const { return plArrayPtr<void*>(m_pStackTrace, (plUInt32)m_uiStackTraceLength); }

    PLASMA_ALWAYS_INLINE plArrayPtr<void*> GetStackTrace() { return plArrayPtr<void*>(m_pStackTrace, (plUInt32)m_uiStackTraceLength); }

    PLASMA_FORCE_INLINE void SetStackTrace(plArrayPtr<void*> stackTrace)
    {
      m_pStackTrace = stackTrace.GetPtr();
      PLASMA_ASSERT_DEV(stackTrace.GetCount() < 0xFFFF, "stack trace too long");
      m_uiStackTraceLength = (plUInt16)stackTrace.GetCount();
    }
  };

  class PLASMA_FOUNDATION_DLL Iterator
  {
  public:
    ~Iterator();

    plAllocatorId Id() const;
    plStringView Name() const;
    plAllocatorId ParentId() const;
    const plAllocatorBase::Stats& Stats() const;

    void Next();
    bool IsValid() const;

    PLASMA_ALWAYS_INLINE void operator++() { Next(); }

  private:
    friend class plMemoryTracker;

    PLASMA_ALWAYS_INLINE Iterator(void* pData)
      : m_pData(pData)
    {
    }

    void* m_pData;
  };

  static plAllocatorId RegisterAllocator(plStringView sName, plBitflags<plMemoryTrackingFlags> flags, plAllocatorId parentId);
  static void DeregisterAllocator(plAllocatorId allocatorId);

  static void AddAllocation(
    plAllocatorId allocatorId, plBitflags<plMemoryTrackingFlags> flags, const void* pPtr, size_t uiSize, size_t uiAlign, plTime allocationTime);
  static void RemoveAllocation(plAllocatorId allocatorId, const void* pPtr);
  static void RemoveAllAllocations(plAllocatorId allocatorId);
  static void SetAllocatorStats(plAllocatorId allocatorId, const plAllocatorBase::Stats& stats);

  static void ResetPerFrameAllocatorStats();

  static plStringView GetAllocatorName(plAllocatorId allocatorId);
  static const plAllocatorBase::Stats& GetAllocatorStats(plAllocatorId allocatorId);
  static plAllocatorId GetAllocatorParentId(plAllocatorId allocatorId);
  static const AllocationInfo& GetAllocationInfo(plAllocatorId allocatorId, const void* pPtr);

  static void DumpMemoryLeaks();

  static Iterator GetIterator();
};
