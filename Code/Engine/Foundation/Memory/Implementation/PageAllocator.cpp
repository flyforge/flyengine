#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>

plAllocatorId plPageAllocator::GetId()
{
  static plAllocatorId id;

  if (id.IsInvalidated())
  {
    id = plMemoryTracker::RegisterAllocator("Page", plAllocatorTrackingMode::Default, plAllocatorId());
  }

  return id;
}


