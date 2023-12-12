#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/HeapAllocation.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

namespace
{
  // no tracking for the tracker data itself
  using TrackerDataAllocator = plAllocator<plMemoryPolicies::plHeapAllocation, 0>;

  static TrackerDataAllocator* s_pTrackerDataAllocator;

  struct TrackerDataAllocatorWrapper
  {
    PLASMA_ALWAYS_INLINE static plAllocatorBase* GetAllocator() { return s_pTrackerDataAllocator; }
  };


  struct AllocatorData
  {
    PLASMA_ALWAYS_INLINE AllocatorData() = default;

    plHybridString<32, TrackerDataAllocatorWrapper> m_sName;
    plBitflags<plMemoryTrackingFlags> m_Flags;

    plAllocatorId m_ParentId;

    plAllocatorBase::Stats m_Stats;

    plHashTable<const void*, plMemoryTracker::AllocationInfo, plHashHelper<const void*>, TrackerDataAllocatorWrapper> m_Allocations;
  };

  struct TrackerData
  {
    PLASMA_ALWAYS_INLINE void Lock() { m_Mutex.Lock(); }
    PLASMA_ALWAYS_INLINE void Unlock() { m_Mutex.Unlock(); }

    plMutex m_Mutex;

    using AllocatorTable = plIdTable<plAllocatorId, AllocatorData, TrackerDataAllocatorWrapper>;
    AllocatorTable m_AllocatorData;

    plAllocatorId m_StaticAllocatorId;
  };

  static TrackerData* s_pTrackerData;
  static bool s_bIsInitialized = false;
  static bool s_bIsInitializing = false;

  static void Initialize()
  {
    if (s_bIsInitialized)
      return;

    PLASMA_ASSERT_DEV(!s_bIsInitializing, "MemoryTracker initialization entered recursively");
    s_bIsInitializing = true;

    if (s_pTrackerDataAllocator == nullptr)
    {
      alignas(PLASMA_ALIGNMENT_OF(TrackerDataAllocator)) static plUInt8 TrackerDataAllocatorBuffer[sizeof(TrackerDataAllocator)];
      s_pTrackerDataAllocator = new (TrackerDataAllocatorBuffer) TrackerDataAllocator("MemoryTracker");
      PLASMA_ASSERT_DEV(s_pTrackerDataAllocator != nullptr, "MemoryTracker initialization failed");
    }

    if (s_pTrackerData == nullptr)
    {
      alignas(PLASMA_ALIGNMENT_OF(TrackerData)) static plUInt8 TrackerDataBuffer[sizeof(TrackerData)];
      s_pTrackerData = new (TrackerDataBuffer) TrackerData();
      PLASMA_ASSERT_DEV(s_pTrackerData != nullptr, "MemoryTracker initialization failed");
    }

    s_bIsInitialized = true;
    s_bIsInitializing = false;
  }

  static void DumpLeak(const plMemoryTracker::AllocationInfo& info, const char* szAllocatorName)
  {
    char szBuffer[512];
    plUInt64 uiSize = info.m_uiSize;
    plStringUtils::snprintf(szBuffer, PLASMA_ARRAY_SIZE(szBuffer), "Leaked %llu bytes allocated by '%s'\n", uiSize, szAllocatorName);

    plLog::Print(szBuffer);

    if (info.GetStackTrace().GetPtr() != nullptr)
    {
      plStackTracer::ResolveStackTrace(info.GetStackTrace(), &plLog::Print);
    }

    plLog::Print("--------------------------------------------------------------------\n\n");
  }
} // namespace

// Iterator
#define CAST_ITER(ptr) static_cast<TrackerData::AllocatorTable::Iterator*>(ptr)

plAllocatorId plMemoryTracker::Iterator::Id() const
{
  return CAST_ITER(m_pData)->Id();
}

plStringView plMemoryTracker::Iterator::Name() const
{
  return CAST_ITER(m_pData)->Value().m_sName;
}

plAllocatorId plMemoryTracker::Iterator::ParentId() const
{
  return CAST_ITER(m_pData)->Value().m_ParentId;
}

const plAllocatorBase::Stats& plMemoryTracker::Iterator::Stats() const
{
  return CAST_ITER(m_pData)->Value().m_Stats;
}

void plMemoryTracker::Iterator::Next()
{
  CAST_ITER(m_pData)->Next();
}

bool plMemoryTracker::Iterator::IsValid() const
{
  return CAST_ITER(m_pData)->IsValid();
}

plMemoryTracker::Iterator::~Iterator()
{
  auto it = CAST_ITER(m_pData);
  PLASMA_DELETE(s_pTrackerDataAllocator, it);
  m_pData = nullptr;
}


// static
plAllocatorId plMemoryTracker::RegisterAllocator(plStringView sName, plBitflags<plMemoryTrackingFlags> flags, plAllocatorId parentId)
{
  Initialize();

  PLASMA_LOCK(*s_pTrackerData);

  AllocatorData data;
  data.m_sName = sName;
  data.m_Flags = flags;
  data.m_ParentId = parentId;

  plAllocatorId id = s_pTrackerData->m_AllocatorData.Insert(data);

  if (data.m_sName == PLASMA_STATIC_ALLOCATOR_NAME)
  {
    s_pTrackerData->m_StaticAllocatorId = id;
  }

  return id;
}

// static
void plMemoryTracker::DeregisterAllocator(plAllocatorId allocatorId)
{
  PLASMA_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

  plUInt32 uiLiveAllocations = data.m_Allocations.GetCount();
  if (uiLiveAllocations != 0)
  {
    for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
    {
      DumpLeak(it.Value(), data.m_sName.GetData());
    }

    PLASMA_REPORT_FAILURE("Allocator '{0}' leaked {1} allocation(s)", data.m_sName.GetData(), uiLiveAllocations);
  }

  s_pTrackerData->m_AllocatorData.Remove(allocatorId);
}

// static
void plMemoryTracker::AddAllocation(plAllocatorId allocatorId, plBitflags<plMemoryTrackingFlags> flags, const void* pPtr, size_t uiSize, size_t uiAlign, plTime allocationTime)
{
  PLASMA_ASSERT_DEV((flags & plMemoryTrackingFlags::EnableAllocationTracking) != 0, "Allocation tracking is turned off, but plMemoryTracker::AddAllocation() is called anyway.");

  PLASMA_ASSERT_DEV(uiAlign < 0xFFFF, "Alignment too big");

  plArrayPtr<void*> stackTrace;
  if (flags.IsSet(plMemoryTrackingFlags::EnableStackTrace))
  {
    void* pBuffer[64];
    plArrayPtr<void*> tempTrace(pBuffer);
    const plUInt32 uiNumTraces = plStackTracer::GetStackTrace(tempTrace);

    stackTrace = PLASMA_NEW_ARRAY(s_pTrackerDataAllocator, void*, uiNumTraces);
    plMemoryUtils::Copy(stackTrace.GetPtr(), pBuffer, uiNumTraces);
  }

  {
    PLASMA_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
    data.m_Stats.m_uiNumAllocations++;
    data.m_Stats.m_uiAllocationSize += uiSize;
    data.m_Stats.m_uiPerFrameAllocationSize += uiSize;
    data.m_Stats.m_PerFrameAllocationTime += allocationTime;

    PLASMA_ASSERT_DEBUG(data.m_Flags == flags, "Given flags have to be identical to allocator flags");
    auto pInfo = &data.m_Allocations[pPtr];
    pInfo->m_uiSize = uiSize;
    pInfo->m_uiAlignment = (plUInt16)uiAlign;
    pInfo->SetStackTrace(stackTrace);
  }
}

// static
void plMemoryTracker::RemoveAllocation(plAllocatorId allocatorId, const void* pPtr)
{
  plArrayPtr<void*> stackTrace;

  {
    PLASMA_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

    AllocationInfo info;
    if (data.m_Allocations.Remove(pPtr, &info))
    {
      data.m_Stats.m_uiNumDeallocations++;
      data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

      stackTrace = info.GetStackTrace();
    }
    else
    {
      PLASMA_REPORT_FAILURE("Invalid Allocation '{0}'. Memory corruption?", plArgP(pPtr));
    }
  }

  PLASMA_DELETE_ARRAY(s_pTrackerDataAllocator, stackTrace);
}

// static
void plMemoryTracker::RemoveAllAllocations(plAllocatorId allocatorId)
{
  PLASMA_LOCK(*s_pTrackerData);
  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    auto& info = it.Value();
    data.m_Stats.m_uiNumDeallocations++;
    data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

    PLASMA_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  data.m_Allocations.Clear();
}

// static
void plMemoryTracker::SetAllocatorStats(plAllocatorId allocatorId, const plAllocatorBase::Stats& stats)
{
  PLASMA_LOCK(*s_pTrackerData);

  s_pTrackerData->m_AllocatorData[allocatorId].m_Stats = stats;
}

// static
void plMemoryTracker::ResetPerFrameAllocatorStats()
{
  PLASMA_LOCK(*s_pTrackerData);

  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    AllocatorData& data = it.Value();
    data.m_Stats.m_uiPerFrameAllocationSize = 0;
    data.m_Stats.m_PerFrameAllocationTime.SetZero();
  }
}

// static
plStringView plMemoryTracker::GetAllocatorName(plAllocatorId allocatorId)
{
  PLASMA_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_sName;
}

// static
const plAllocatorBase::Stats& plMemoryTracker::GetAllocatorStats(plAllocatorId allocatorId)
{
  PLASMA_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_Stats;
}

// static
plAllocatorId plMemoryTracker::GetAllocatorParentId(plAllocatorId allocatorId)
{
  PLASMA_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_ParentId;
}

// static
const plMemoryTracker::AllocationInfo& plMemoryTracker::GetAllocationInfo(plAllocatorId allocatorId, const void* pPtr)
{
  PLASMA_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  const AllocationInfo* info = nullptr;
  if (data.m_Allocations.TryGetValue(pPtr, info))
  {
    return *info;
  }

  static AllocationInfo invalidInfo;

  PLASMA_REPORT_FAILURE("Could not find info for allocation {0}", plArgP(pPtr));
  return invalidInfo;
}


struct LeakInfo
{
  PLASMA_DECLARE_POD_TYPE();

  plAllocatorId m_AllocatorId;
  size_t m_uiSize = 0;
  const void* m_pParentLeak = nullptr;

  PLASMA_ALWAYS_INLINE bool IsRootLeak() const { return m_pParentLeak == nullptr && m_AllocatorId != s_pTrackerData->m_StaticAllocatorId; }
};

// static
void plMemoryTracker::DumpMemoryLeaks()
{
  if (s_pTrackerData == nullptr) // if both tracking and tracing is disabled there is no tracker data
    return;
  PLASMA_LOCK(*s_pTrackerData);

  static plHashTable<const void*, LeakInfo, plHashHelper<const void*>, TrackerDataAllocatorWrapper> leakTable;
  leakTable.Clear();

  // first collect all leaks
  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    const AllocatorData& data = it.Value();
    for (auto it2 = data.m_Allocations.GetIterator(); it2.IsValid(); ++it2)
    {
      LeakInfo leak;
      leak.m_AllocatorId = it.Id();
      leak.m_uiSize = it2.Value().m_uiSize;
      leak.m_pParentLeak = nullptr;

      leakTable.Insert(it2.Key(), leak);
    }
  }

  // find dependencies
  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    const void* curPtr = ptr;
    const void* endPtr = plMemoryUtils::AddByteOffset(ptr, leak.m_uiSize);

    while (curPtr < endPtr)
    {
      const void* testPtr = *reinterpret_cast<const void* const*>(curPtr);

      LeakInfo* dependentLeak = nullptr;
      if (leakTable.TryGetValue(testPtr, dependentLeak))
      {
        dependentLeak->m_pParentLeak = ptr;
      }

      curPtr = plMemoryUtils::AddByteOffset(curPtr, sizeof(void*));
    }
  }

  // dump leaks
  plUInt64 uiNumLeaks = 0;

  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    if (leak.IsRootLeak())
    {
      if (uiNumLeaks == 0)
      {
        plLog::Print("\n\n--------------------------------------------------------------------\n"
                     "Memory Leak Report:"
                     "\n--------------------------------------------------------------------\n\n");
      }

      const AllocatorData& data = s_pTrackerData->m_AllocatorData[leak.m_AllocatorId];
      plMemoryTracker::AllocationInfo info;
      data.m_Allocations.TryGetValue(ptr, info);

      DumpLeak(info, data.m_sName.GetData());

      ++uiNumLeaks;
    }
  }

  if (uiNumLeaks > 0)
  {
    plLog::Printf("\n--------------------------------------------------------------------\n"
                  "Found %llu root memory leak(s)."
                  "\n--------------------------------------------------------------------\n\n",
      uiNumLeaks);

    PLASMA_REPORT_FAILURE("Found {0} root memory leak(s).", uiNumLeaks);
  }
}

// static
plMemoryTracker::Iterator plMemoryTracker::GetIterator()
{
  auto pInnerIt = PLASMA_NEW(s_pTrackerDataAllocator, TrackerData::AllocatorTable::Iterator, s_pTrackerData->m_AllocatorData.GetIterator());
  return Iterator(pInnerIt);
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_MemoryTracker);
