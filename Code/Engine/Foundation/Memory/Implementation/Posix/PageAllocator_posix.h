
#include <Foundation/Time/Time.h>

// static
void* plPageAllocator::AllocatePage(size_t uiSize)
{
  plTime fAllocationTime = plTime::Now();

  void* ptr = nullptr;
  size_t uiAlign = plSystemInformation::Get().GetMemoryPageSize();
  const int res = posix_memalign(&ptr, uiAlign, uiSize);
  PLASMA_ASSERT_DEBUG(res == 0, "Failed to align pointer");
  PLASMA_IGNORE_UNUSED(res);

  PLASMA_CHECK_ALIGNMENT(ptr, uiAlign);

  if ((plMemoryTrackingFlags::Default & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::AddAllocation(GetPageAllocatorId(), plMemoryTrackingFlags::Default, ptr, uiSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return ptr;
}

// static
void plPageAllocator::DeallocatePage(void* ptr)
{
  if ((plMemoryTrackingFlags::Default & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::RemoveAllocation(GetPageAllocatorId(), ptr);
  }

  free(ptr);
}
