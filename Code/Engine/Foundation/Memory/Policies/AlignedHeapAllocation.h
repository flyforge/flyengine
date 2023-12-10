#pragma once

#include <Foundation/Basics.h>

namespace plMemoryPolicies
{
  /// \brief Aligned Heap memory allocation policy.
  ///
  /// \see plAllocator
  class plAlignedHeapAllocation
  {
  public:
    PLASMA_ALWAYS_INLINE plAlignedHeapAllocation(plAllocatorBase* pParent) {}
    PLASMA_ALWAYS_INLINE ~plAlignedHeapAllocation() = default;

    void* Allocate(size_t uiSize, size_t uiAlign);
    void Deallocate(void* pPtr);

    PLASMA_ALWAYS_INLINE plAllocatorBase* GetParent() const { return nullptr; }
  };

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Memory/Policies/Win/AlignedHeapAllocation_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Memory/Policies/Posix/AlignedHeapAllocation_posix.h>
#else
#  error "plAlignedHeapAllocation is not implemented on current platform"
#endif
} // namespace plMemoryPolicies
