#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/ThreadUtils.h>

PL_MAKE_MEMBERFUNCTION_CHECKER(Reallocate, plHasReallocate);

#include <Foundation/Memory/Implementation/AllocatorMixin_inl.h>

/// \brief Policy based allocator implementation of the plAllocator interface.
///
/// AllocationPolicy defines how the actual memory is allocated.\n
/// TrackingFlags defines how stats about allocations are tracked.\n
template <typename AllocationPolicy, plAllocatorTrackingMode TrackingMode = plAllocatorTrackingMode::Default>
class plAllocatorWithPolicy : public plInternal::plAllocatorMixinReallocate<AllocationPolicy, TrackingMode,
                      plHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>
{
public:
  plAllocatorWithPolicy(plStringView sName, plAllocator* pParent = nullptr)
    : plInternal::plAllocatorMixinReallocate<AllocationPolicy, TrackingMode,
        plHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>(sName, pParent)
  {
  }
};
