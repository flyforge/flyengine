#pragma once

#include <Foundation/Basics.h>

namespace plMemoryPolicies
{
  /// \brief Default heap memory allocation policy.
  ///
  /// \see plAllocator
  class plHeapAllocation
  {
  public:
    PLASMA_ALWAYS_INLINE plHeapAllocation(plAllocatorBase* pParent) {}
    PLASMA_ALWAYS_INLINE ~plHeapAllocation() = default;

    PLASMA_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
      // if these asserts fail, you need to check what container made the allocation and change it
      // to use an aligned allocator, e.g. plAlignedAllocatorWrapper

      // unfortunately using PLASMA_ALIGNMENT_MINIMUM doesn't work, because even on 32 Bit systems we try to do allocations with 8 Byte
      // alignment interestingly, the code that does that, seems to work fine anyway
      PLASMA_ASSERT_DEBUG(
        uiAlign <= 8, "This allocator does not guarantee alignments larger than 8. Use an aligned allocator to allocate the desired data type.");

      void* ptr = malloc(PadSize(uiSize));
      PLASMA_CHECK_ALIGNMENT(ptr, uiAlign);

      return OffsetPtr(ptr);
    }

    PLASMA_FORCE_INLINE void* Reallocate(void* pCurrentPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
    {
      void* ptr = realloc(RestorePtr(pCurrentPtr), PadSize(uiNewSize));
      PLASMA_CHECK_ALIGNMENT(ptr, uiAlign);

      return OffsetPtr(ptr);
    }

    PLASMA_ALWAYS_INLINE void Deallocate(void* pPtr) { free(RestorePtr(pPtr)); }

    PLASMA_ALWAYS_INLINE plAllocatorBase* GetParent() const { return nullptr; }

  private:
    PLASMA_ALWAYS_INLINE size_t PadSize(size_t uiSize)
    {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
      return uiSize + 2 * PLASMA_ALIGNMENT_MINIMUM;
#else
      return uiSize;
#endif
    }

    PLASMA_ALWAYS_INLINE void* OffsetPtr(void* ptr)
    {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
      plUInt32 uiOffset = plMemoryUtils::IsAligned(ptr, 2 * PLASMA_ALIGNMENT_MINIMUM) ? PLASMA_ALIGNMENT_MINIMUM : 2 * PLASMA_ALIGNMENT_MINIMUM;
      ptr = plMemoryUtils::AddByteOffset(ptr, uiOffset - 4);
      *static_cast<plUInt32*>(ptr) = uiOffset;
      return plMemoryUtils::AddByteOffset(ptr, 4);
#else
      return ptr;
#endif
    }

    PLASMA_ALWAYS_INLINE void* RestorePtr(void* ptr)
    {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
      ptr = plMemoryUtils::AddByteOffset(ptr, -4);
      plInt32 uiOffset = *static_cast<plUInt32*>(ptr);
      return plMemoryUtils::AddByteOffset(ptr, -uiOffset + 4);
#else
      return ptr;
#endif
    }
  };
} // namespace plMemoryPolicies
