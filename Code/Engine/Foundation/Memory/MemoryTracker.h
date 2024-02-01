#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>

enum class plAllocatorTrackingMode : plUInt32
{
  Nothing,                       ///< The allocator doesn't track anything. Use this for best performance.
  Basics,                        ///< The allocator will be known to the system, so it can show up in debugging tools, but barely anything more.
  AllocationStats,               ///< The allocator keeps track of how many allocations and deallocations it did and how large its memory usage is.
  AllocationStatsIgnoreLeaks,    ///< Same as AllocationStats, but any remaining allocations at shutdown are not reported as leaks.
  AllocationStatsAndStacktraces, ///< The allocator will record stack traces for each allocation, which can be used to find memory leaks.

  Default = PL_ALLOC_TRACKING_DEFAULT,
};

/// \brief Memory tracker which keeps track of all allocations and constructions
class PL_FOUNDATION_DLL plMemoryTracker
{
public:
  struct AllocationInfo
  {
    PL_DECLARE_POD_TYPE();

    PL_FORCE_INLINE AllocationInfo()

      = default;

    void** m_pStackTrace = nullptr;
    size_t m_uiSize = 0;
    plUInt16 m_uiAlignment = 0;
    plUInt16 m_uiStackTraceLength = 0;

    PL_ALWAYS_INLINE const plArrayPtr<void*> GetStackTrace() const { return plArrayPtr<void*>(m_pStackTrace, (plUInt32)m_uiStackTraceLength); }

    PL_ALWAYS_INLINE plArrayPtr<void*> GetStackTrace() { return plArrayPtr<void*>(m_pStackTrace, (plUInt32)m_uiStackTraceLength); }

    PL_FORCE_INLINE void SetStackTrace(plArrayPtr<void*> stackTrace)
    {
      m_pStackTrace = stackTrace.GetPtr();
      PL_ASSERT_DEV(stackTrace.GetCount() < 0xFFFF, "stack trace too long");
      m_uiStackTraceLength = (plUInt16)stackTrace.GetCount();
    }
  };

  class PL_FOUNDATION_DLL Iterator
  {
  public:
    ~Iterator();

    plAllocatorId Id() const;
    plStringView Name() const;
    plAllocatorId ParentId() const;
    const plAllocator::Stats& Stats() const;

    void Next();
    bool IsValid() const;

    PL_ALWAYS_INLINE void operator++() { Next(); }

  private:
    friend class plMemoryTracker;

    PL_ALWAYS_INLINE Iterator(void* pData)
      : m_pData(pData)
    {
    }

    void* m_pData;
  };

  static plAllocatorId RegisterAllocator(plStringView sName, plAllocatorTrackingMode mode, plAllocatorId parentId);
  static void DeregisterAllocator(plAllocatorId allocatorId);

  static void AddAllocation(plAllocatorId allocatorId, plAllocatorTrackingMode mode, const void* pPtr, size_t uiSize, size_t uiAlign, plTime allocationTime);
  static void RemoveAllocation(plAllocatorId allocatorId, const void* pPtr);
  static void RemoveAllAllocations(plAllocatorId allocatorId);
  static void SetAllocatorStats(plAllocatorId allocatorId, const plAllocator::Stats& stats);

  static void ResetPerFrameAllocatorStats();

  static plStringView GetAllocatorName(plAllocatorId allocatorId);
  static const plAllocator::Stats& GetAllocatorStats(plAllocatorId allocatorId);
  static plAllocatorId GetAllocatorParentId(plAllocatorId allocatorId);
  static const AllocationInfo& GetAllocationInfo(plAllocatorId allocatorId, const void* pPtr);

  static Iterator GetIterator();

  /// \brief Callback for printing strings.
  using PrintFunc = void (*)(const char* szLine);

  /// \brief Reports back information about all currently known root memory leaks.
  ///
  /// Returns the number of found memory leaks.
  static plUInt32 PrintMemoryLeaks(PrintFunc printfunc);

  /// \brief Prints the known memory leaks to plLog and triggers an assert if there are any.
  ///
  /// This is useful to call at the end of an application, to get a debug breakpoint in case of memory leaks.
  static void DumpMemoryLeaks();
};
