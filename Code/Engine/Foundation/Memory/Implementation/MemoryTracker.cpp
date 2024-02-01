#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyHeap.h>
#include <Foundation/Strings/String.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace
{
  // no tracking for the tracker data itself
  using TrackerDataAllocator = plAllocatorWithPolicy<plAllocPolicyHeap, plAllocatorTrackingMode::Nothing>;

  static TrackerDataAllocator* s_pTrackerDataAllocator;

  struct TrackerDataAllocatorWrapper
  {
    PL_ALWAYS_INLINE static plAllocator* GetAllocator() { return s_pTrackerDataAllocator; }
  };


  struct AllocatorData
  {
    PL_ALWAYS_INLINE AllocatorData() = default;

    plHybridString<32, TrackerDataAllocatorWrapper> m_sName;
    plAllocatorTrackingMode m_TrackingMode;

    plAllocatorId m_ParentId;

    plAllocator::Stats m_Stats;

    plHashTable<const void*, plMemoryTracker::AllocationInfo, plHashHelper<const void*>, TrackerDataAllocatorWrapper> m_Allocations;
  };

  struct TrackerData
  {
    PL_ALWAYS_INLINE void Lock() { m_Mutex.Lock(); }
    PL_ALWAYS_INLINE void Unlock() { m_Mutex.Unlock(); }

    plMutex m_Mutex;

    using AllocatorTable = plIdTable<plAllocatorId, AllocatorData, TrackerDataAllocatorWrapper>;
    AllocatorTable m_AllocatorData;
  };

  static TrackerData* s_pTrackerData;
  static bool s_bIsInitialized = false;
  static bool s_bIsInitializing = false;

  static void Initialize()
  {
    if (s_bIsInitialized)
      return;

    PL_ASSERT_DEV(!s_bIsInitializing, "MemoryTracker initialization entered recursively");
    s_bIsInitializing = true;

    if (s_pTrackerDataAllocator == nullptr)
    {
      alignas(PL_ALIGNMENT_OF(TrackerDataAllocator)) static plUInt8 TrackerDataAllocatorBuffer[sizeof(TrackerDataAllocator)];
      s_pTrackerDataAllocator = new (TrackerDataAllocatorBuffer) TrackerDataAllocator("MemoryTracker");
      PL_ASSERT_DEV(s_pTrackerDataAllocator != nullptr, "MemoryTracker initialization failed");
    }

    if (s_pTrackerData == nullptr)
    {
      alignas(PL_ALIGNMENT_OF(TrackerData)) static plUInt8 TrackerDataBuffer[sizeof(TrackerData)];
      s_pTrackerData = new (TrackerDataBuffer) TrackerData();
      PL_ASSERT_DEV(s_pTrackerData != nullptr, "MemoryTracker initialization failed");
    }

    s_bIsInitialized = true;
    s_bIsInitializing = false;
  }

  static void DumpLeak(const plMemoryTracker::AllocationInfo& info, const char* szAllocatorName)
  {
    char szBuffer[512];
    plUInt64 uiSize = info.m_uiSize;
    plStringUtils::snprintf(szBuffer, PL_ARRAY_SIZE(szBuffer), "Leaked %llu bytes allocated by '%s'\n", uiSize, szAllocatorName);

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

const plAllocator::Stats& plMemoryTracker::Iterator::Stats() const
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
  PL_DELETE(s_pTrackerDataAllocator, it);
  m_pData = nullptr;
}


// static
plAllocatorId plMemoryTracker::RegisterAllocator(plStringView sName, plAllocatorTrackingMode mode, plAllocatorId parentId)
{
  Initialize();

  PL_LOCK(*s_pTrackerData);

  AllocatorData data;
  data.m_sName = sName;
  data.m_TrackingMode = mode;
  data.m_ParentId = parentId;

  return s_pTrackerData->m_AllocatorData.Insert(data);
}

// static
void plMemoryTracker::DeregisterAllocator(plAllocatorId allocatorId)
{
  PL_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];

  plUInt32 uiLiveAllocations = data.m_Allocations.GetCount();
  if (uiLiveAllocations != 0)
  {
    for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
    {
      DumpLeak(it.Value(), data.m_sName.GetData());
    }

    PL_REPORT_FAILURE("Allocator '{0}' leaked {1} allocation(s)", data.m_sName.GetData(), uiLiveAllocations);
  }

  s_pTrackerData->m_AllocatorData.Remove(allocatorId);
}

// static
void plMemoryTracker::AddAllocation(plAllocatorId allocatorId, plAllocatorTrackingMode mode, const void* pPtr, size_t uiSize, size_t uiAlign, plTime allocationTime)
{
  PL_ASSERT_DEV(uiAlign < 0xFFFF, "Alignment too big");

  plArrayPtr<void*> stackTrace;
  if (mode >= plAllocatorTrackingMode::AllocationStatsAndStacktraces)
  {
    void* pBuffer[64];
    plArrayPtr<void*> tempTrace(pBuffer);
    const plUInt32 uiNumTraces = plStackTracer::GetStackTrace(tempTrace);

    stackTrace = PL_NEW_ARRAY(s_pTrackerDataAllocator, void*, uiNumTraces);
    plMemoryUtils::Copy(stackTrace.GetPtr(), pBuffer, uiNumTraces);
  }

  {
    PL_LOCK(*s_pTrackerData);

    AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
    data.m_Stats.m_uiNumAllocations++;
    data.m_Stats.m_uiAllocationSize += uiSize;
    data.m_Stats.m_uiPerFrameAllocationSize += uiSize;
    data.m_Stats.m_PerFrameAllocationTime += allocationTime;

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
    PL_LOCK(*s_pTrackerData);

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
      PL_REPORT_FAILURE("Invalid Allocation '{0}'. Memory corruption?", plArgP(pPtr));
    }
  }

  PL_DELETE_ARRAY(s_pTrackerDataAllocator, stackTrace);
}

// static
void plMemoryTracker::RemoveAllAllocations(plAllocatorId allocatorId)
{
  PL_LOCK(*s_pTrackerData);
  AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  for (auto it = data.m_Allocations.GetIterator(); it.IsValid(); ++it)
  {
    auto& info = it.Value();
    data.m_Stats.m_uiNumDeallocations++;
    data.m_Stats.m_uiAllocationSize -= info.m_uiSize;

    PL_DELETE_ARRAY(s_pTrackerDataAllocator, info.GetStackTrace());
  }
  data.m_Allocations.Clear();
}

// static
void plMemoryTracker::SetAllocatorStats(plAllocatorId allocatorId, const plAllocator::Stats& stats)
{
  PL_LOCK(*s_pTrackerData);

  s_pTrackerData->m_AllocatorData[allocatorId].m_Stats = stats;
}

// static
void plMemoryTracker::ResetPerFrameAllocatorStats()
{
  PL_LOCK(*s_pTrackerData);

  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    AllocatorData& data = it.Value();
    data.m_Stats.m_uiPerFrameAllocationSize = 0;
    data.m_Stats.m_PerFrameAllocationTime = plTime::MakeZero();
  }
}

// static
plStringView plMemoryTracker::GetAllocatorName(plAllocatorId allocatorId)
{
  PL_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_sName;
}

// static
const plAllocator::Stats& plMemoryTracker::GetAllocatorStats(plAllocatorId allocatorId)
{
  PL_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_Stats;
}

// static
plAllocatorId plMemoryTracker::GetAllocatorParentId(plAllocatorId allocatorId)
{
  PL_LOCK(*s_pTrackerData);

  return s_pTrackerData->m_AllocatorData[allocatorId].m_ParentId;
}

// static
const plMemoryTracker::AllocationInfo& plMemoryTracker::GetAllocationInfo(plAllocatorId allocatorId, const void* pPtr)
{
  PL_LOCK(*s_pTrackerData);

  const AllocatorData& data = s_pTrackerData->m_AllocatorData[allocatorId];
  const AllocationInfo* info = nullptr;
  if (data.m_Allocations.TryGetValue(pPtr, info))
  {
    return *info;
  }

  static AllocationInfo invalidInfo;

  PL_REPORT_FAILURE("Could not find info for allocation {0}", plArgP(pPtr));
  return invalidInfo;
}

struct LeakInfo
{
  PL_DECLARE_POD_TYPE();

  plAllocatorId m_AllocatorId;
  size_t m_uiSize = 0;
  bool m_bIsRootLeak = true;
};

// static
plUInt32 plMemoryTracker::PrintMemoryLeaks(PrintFunc printfunc)
{
  if (s_pTrackerData == nullptr) // if both tracking and tracing is disabled there is no tracker data
    return 0;

  PL_LOCK(*s_pTrackerData);

  plHashTable<const void*, LeakInfo, plHashHelper<const void*>, TrackerDataAllocatorWrapper> leakTable;

  // first collect all leaks
  for (auto it = s_pTrackerData->m_AllocatorData.GetIterator(); it.IsValid(); ++it)
  {
    const AllocatorData& data = it.Value();
    for (auto it2 = data.m_Allocations.GetIterator(); it2.IsValid(); ++it2)
    {
      LeakInfo leak;
      leak.m_AllocatorId = it.Id();
      leak.m_uiSize = it2.Value().m_uiSize;

      if (data.m_TrackingMode == plAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
      {
        leak.m_bIsRootLeak = false;
      }

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
        dependentLeak->m_bIsRootLeak = false;
      }

      curPtr = plMemoryUtils::AddByteOffset(curPtr, sizeof(void*));
    }
  }

  // dump leaks
  plUInt32 uiNumLeaks = 0;

  for (auto it = leakTable.GetIterator(); it.IsValid(); ++it)
  {
    const void* ptr = it.Key();
    const LeakInfo& leak = it.Value();

    if (leak.m_bIsRootLeak)
    {
      const AllocatorData& data = s_pTrackerData->m_AllocatorData[leak.m_AllocatorId];

      if (data.m_TrackingMode != plAllocatorTrackingMode::AllocationStatsIgnoreLeaks)
      {
        if (uiNumLeaks == 0)
        {
          printfunc("\n\n--------------------------------------------------------------------\n"
                    "Memory Leak Report:"
                    "\n--------------------------------------------------------------------\n\n");
        }

        plMemoryTracker::AllocationInfo info;
        data.m_Allocations.TryGetValue(ptr, info);

        DumpLeak(info, data.m_sName.GetData());

        ++uiNumLeaks;
      }
    }
  }

  if (uiNumLeaks > 0)
  {
    char tmp[1024];
    plStringUtils::snprintf(tmp, 1024, "\n--------------------------------------------------------------------\n"
                                       "Found %u root memory leak(s)."
                                       "\n--------------------------------------------------------------------\n\n",
      uiNumLeaks);

    printfunc(tmp);
  }

  return uiNumLeaks;
}

// static
void plMemoryTracker::DumpMemoryLeaks()
{
  const plUInt32 uiNumLeaks = PrintMemoryLeaks(plLog::Print);

  if (uiNumLeaks > 0)
  {
    PL_REPORT_FAILURE("Found {0} root memory leak(s). See console output for details.", uiNumLeaks);
  }
}

// static
plMemoryTracker::Iterator plMemoryTracker::GetIterator()
{
  auto pInnerIt = PL_NEW(s_pTrackerDataAllocator, TrackerData::AllocatorTable::Iterator, s_pTrackerData->m_AllocatorData.GetIterator());
  return Iterator(pInnerIt);
}
