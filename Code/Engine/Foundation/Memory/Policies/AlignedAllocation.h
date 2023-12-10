#pragma once

#include <Foundation/Math/Math.h>

namespace plMemoryPolicies
{
  /// \brief Allocation policy to support custom alignment per allocation.
  ///
  /// \see plAllocator
  template <typename T>
  class plAlignedAllocation
  {
  public:
    plAlignedAllocation(plAllocatorBase* pParent)
      : m_allocator(pParent)
    {
    }

    void* Allocate(size_t uiSize, size_t uiAlign)
    {
      PLASMA_ASSERT_DEV(uiAlign < (1 << 24), "Alignment of {0} is too big. Maximum supported alignment is 16MB.", uiAlign);

      const plUInt32 uiPadding = (plUInt32)(uiAlign - 1 + MetadataSize);
      const size_t uiAlignedSize = uiSize + uiPadding;

      plUInt8* pMemory = (plUInt8*)m_allocator.Allocate(uiAlignedSize, PLASMA_ALIGNMENT_MINIMUM);

      plUInt8* pAlignedMemory = plMemoryUtils::AlignBackwards(pMemory + uiPadding, uiAlign);

      plUInt32* pMetadata = GetMetadataPtr(pAlignedMemory);
      *pMetadata = PackMetadata((plUInt32)(pAlignedMemory - pMemory), (plUInt32)uiAlign);

      return pAlignedMemory;
    }

    void Deallocate(void* pPtr)
    {
      const plUInt32 uiOffset = UnpackOffset(GetMetadata(pPtr));
      plUInt8* pMemory = static_cast<plUInt8*>(pPtr) - uiOffset;
      m_allocator.Deallocate(pMemory);
    }

    size_t AllocatedSize(const void* pPtr)
    {
      const plUInt32 uiMetadata = GetMetadata(pPtr);
      const plUInt32 uiOffset = UnpackOffset(uiMetadata);
      const plUInt32 uiAlign = UnpackAlignment(uiMetadata);
      const plUInt32 uiPadding = uiAlign - 1 + MetadataSize;

      const plUInt8* pMemory = static_cast<const plUInt8*>(pPtr) - uiOffset;
      return m_allocator.AllocatedSize(pMemory) - uiPadding;
    }

    size_t UsedMemorySize(const void* pPtr)
    {
      const plUInt32 uiOffset = UnpackOffset(GetMetadata(pPtr));
      const plUInt8* pMemory = static_cast<const plUInt8*>(pPtr) - uiOffset;
      return m_allocator.UsedMemorySize(pMemory);
    }

    PLASMA_ALWAYS_INLINE plAllocatorBase* GetParent() const { return m_allocator.GetParent(); }

  private:
    enum
    {
      MetadataSize = sizeof(plUInt32)
    };

    // Meta-data is stored 4 bytes before the aligned memory
    inline plUInt32* GetMetadataPtr(void* pAlignedMemory)
    {
      return static_cast<plUInt32*>(plMemoryUtils::AddByteOffset(pAlignedMemory, -MetadataSize));
    }

    inline plUInt32 GetMetadata(const void* pAlignedMemory)
    {
      return *static_cast<const plUInt32*>(plMemoryUtils::AddByteOffset(pAlignedMemory, -MetadataSize));
    }

    // Store offset between pMemory and pAlignedMemory in the lower 24 bit of meta-data.
    // The upper 8 bit are used to store the Log2 of the alignment.
    PLASMA_ALWAYS_INLINE plUInt32 PackMetadata(plUInt32 uiOffset, plUInt32 uiAlignment) { return uiOffset | (plMath::Log2i(uiAlignment) << 24); }

    PLASMA_ALWAYS_INLINE plUInt32 UnpackOffset(plUInt32 uiMetadata) { return uiMetadata & 0x00FFFFFF; }

    PLASMA_ALWAYS_INLINE plUInt32 UnpackAlignment(plUInt32 uiMetadata) { return 1 << (uiMetadata >> 24); }

    T m_allocator;
  };
} // namespace plMemoryPolicies
