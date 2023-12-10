#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>

static plAllocatorId GetPageAllocatorId()
{
  static plAllocatorId id;

  if (id.IsInvalidated())
  {
    id = plMemoryTracker::RegisterAllocator("Page", plMemoryTrackingFlags::Default, plAllocatorId());
  }

  return id;
}

plAllocatorId plPageAllocator::GetId()
{
  return GetPageAllocatorId();
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Memory/Implementation/Win/PageAllocator_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Memory/Implementation/Posix/PageAllocator_posix.h>
#else
#  error "plPageAllocator is not implemented on current platform"
#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_PageAllocator);
