#pragma once

#include <Foundation/Basics.h>

/// \brief Aligned Heap memory allocation policy.
///
/// \see plAllocatorWithPolicy
class plAllocPolicyAlignedHeap
{
public:
  PL_ALWAYS_INLINE plAllocPolicyAlignedHeap(plAllocator* pParent) {}
  PL_ALWAYS_INLINE ~plAllocPolicyAlignedHeap() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void Deallocate(void* pPtr);

  PL_ALWAYS_INLINE plAllocator* GetParent() const { return nullptr; }
};

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Memory/Policies/Win/AllocPolicyAlignedHeap_win.h>
#elif PL_ENABLED(PL_PLATFORM_OSX) || PL_ENABLED(PL_PLATFORM_LINUX) || PL_ENABLED(PL_PLATFORM_ANDROID)
#  include <Foundation/Memory/Policies/Posix/AllocPolicyAlignedHeap_posix.h>
#else
#  error "plAllocPolicyAlignedHeap is not implemented on current platform"
#endif
