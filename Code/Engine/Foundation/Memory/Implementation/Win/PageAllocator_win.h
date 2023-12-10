#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/Platform_win.h>
#include <Foundation/Time/Time.h>

// static
void* plPageAllocator::AllocatePage(size_t uiSize)
{
  plTime fAllocationTime = plTime::Now();

  void* ptr = ::VirtualAlloc(nullptr, uiSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  PLASMA_ASSERT_DEV(ptr != nullptr, "Could not allocate memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));

  size_t uiAlign = plSystemInformation::Get().GetMemoryPageSize();
  PLASMA_CHECK_ALIGNMENT(ptr, uiAlign);

  if constexpr ((plMemoryTrackingFlags::Default & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::AddAllocation(GetPageAllocatorId(), plMemoryTrackingFlags::Default, ptr, uiSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return ptr;
}

// static
void plPageAllocator::DeallocatePage(void* pPtr)
{
  if constexpr ((plMemoryTrackingFlags::Default & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::RemoveAllocation(GetPageAllocatorId(), pPtr);
  }

  PLASMA_VERIFY(::VirtualFree(pPtr, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));
}
