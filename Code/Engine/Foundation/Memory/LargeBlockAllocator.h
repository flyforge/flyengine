#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadUtils.h>

/// \brief This struct represents a block of type T, typically 4kb.
template <typename T, plUInt32 SizeInBytes>
struct plDataBlock
{
  PL_DECLARE_POD_TYPE();

  enum
  {
    SIZE_IN_BYTES = SizeInBytes,
    CAPACITY = SIZE_IN_BYTES / sizeof(T)
  };

  plDataBlock(T* pData, plUInt32 uiCount);

  T* ReserveBack();
  T* PopBack();

  bool IsEmpty() const;
  bool IsFull() const;

  T& operator[](plUInt32 uiIndex) const;

  T* m_pData;
  plUInt32 m_uiCount;
};

/// \brief A block allocator which can only allocates blocks of memory at once.
template <plUInt32 BlockSizeInByte>
class plLargeBlockAllocator
{
public:
  plLargeBlockAllocator(plStringView sName, plAllocator* pParent, plAllocatorTrackingMode mode = plAllocatorTrackingMode::Default);
  ~plLargeBlockAllocator();

  template <typename T>
  plDataBlock<T, BlockSizeInByte> AllocateBlock();

  template <typename T>
  void DeallocateBlock(plDataBlock<T, BlockSizeInByte>& ref_block);


  plStringView GetName() const;

  plAllocatorId GetId() const;

  const plAllocator::Stats& GetStats() const;

private:
  void* Allocate(size_t uiAlign);
  void Deallocate(void* ptr);

  plAllocatorId m_Id;
  plAllocatorTrackingMode m_TrackingMode;

  plMutex m_Mutex;
  plThreadID m_ThreadID;

  struct SuperBlock
  {
    PL_DECLARE_POD_TYPE();

    enum
    {
      NUM_BLOCKS = 16,
      SIZE_IN_BYTES = BlockSizeInByte * NUM_BLOCKS
    };

    void* m_pBasePtr;

    plUInt32 m_uiUsedBlocks;
  };

  plDynamicArray<SuperBlock> m_SuperBlocks;
  plDynamicArray<plUInt32> m_FreeBlocks;
};

#include <Foundation/Memory/Implementation/LargeBlockAllocator_inl.h>
