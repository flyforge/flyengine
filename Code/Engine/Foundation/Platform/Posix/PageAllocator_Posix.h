#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Time.h>

// static
void* plPageAllocator::AllocatePage(size_t uiSize)
{
  plTime fAllocationTime = plTime::Now();

  void* ptr = nullptr;
  size_t uiAlign = plSystemInformation::Get().GetMemoryPageSize();
  const int res = posix_memalign(&ptr, uiAlign, uiSize);
  PL_ASSERT_DEBUG(res == 0, "Failed to align pointer");
  PL_IGNORE_UNUSED(res);

  PL_CHECK_ALIGNMENT(ptr, uiAlign);

  if constexpr (plAllocatorTrackingMode::Default >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::AddAllocation(plPageAllocator::GetId(), plAllocatorTrackingMode::Default, ptr, uiSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return ptr;
}

// static
void plPageAllocator::DeallocatePage(void* ptr)
{
  if constexpr (plAllocatorTrackingMode::Default >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::RemoveAllocation(plPageAllocator::GetId(), ptr);
  }

  free(ptr);
}
