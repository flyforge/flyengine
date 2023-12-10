#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorBase.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/ThreadUtils.h>

PLASMA_MAKE_MEMBERFUNCTION_CHECKER(Reallocate, plHasReallocate);

#include <Foundation/Memory/Implementation/Allocator_inl.h>

/// \brief Policy based allocator implementation of the plAllocatorBase interface.
///
/// AllocationPolicy defines how the actual memory is allocated.\n
/// TrackingFlags defines how stats about allocations are tracked.\n
template <typename AllocationPolicy, plUInt32 TrackingFlags = plMemoryTrackingFlags::Default>
class plAllocator : public plInternal::plAllocatorMixinReallocate<AllocationPolicy, TrackingFlags,
                      plHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>
{
public:
  plAllocator(plStringView sName, plAllocatorBase* pParent = nullptr)
    : plInternal::plAllocatorMixinReallocate<AllocationPolicy, TrackingFlags,
        plHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>(sName, pParent)
  {
  }
};
