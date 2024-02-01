#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/Platform_win.h>
#  include <Foundation/Memory/MemoryTracker.h>
#  include <Foundation/Memory/PageAllocator.h>
#  include <Foundation/System/SystemInformation.h>
#  include <Foundation/Time/Time.h>

// static
void* plPageAllocator::AllocatePage(size_t uiSize)
{
  plTime fAllocationTime = plTime::Now();

  void* ptr = ::VirtualAlloc(nullptr, uiSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  PL_ASSERT_DEV(ptr != nullptr, "Could not allocate memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));

  size_t uiAlign = plSystemInformation::Get().GetMemoryPageSize();
  PL_CHECK_ALIGNMENT(ptr, uiAlign);

  if constexpr (plAllocatorTrackingMode::Default >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::AddAllocation(plPageAllocator::GetId(), plAllocatorTrackingMode::Default, ptr, uiSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return ptr;
}

// static
void plPageAllocator::DeallocatePage(void* pPtr)
{
  if constexpr (plAllocatorTrackingMode::Default >= plAllocatorTrackingMode::AllocationStats)
  {
    plMemoryTracker::RemoveAllocation(plPageAllocator::GetId(), pPtr);
  }

  PL_VERIFY(::VirtualFree(pPtr, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", plArgErrorCode(::GetLastError()));
}

#endif


