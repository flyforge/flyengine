
template <typename T, plUInt32 SizeInBytes>
PLASMA_ALWAYS_INLINE plDataBlock<T, SizeInBytes>::plDataBlock(T* pData, plUInt32 uiCount)
{
  m_pData = pData;
  m_uiCount = uiCount;
}

template <typename T, plUInt32 SizeInBytes>
PLASMA_FORCE_INLINE T* plDataBlock<T, SizeInBytes>::ReserveBack()
{
  PLASMA_ASSERT_DEV(m_uiCount < CAPACITY, "Block is full.");
  return m_pData + m_uiCount++;
}

template <typename T, plUInt32 SizeInBytes>
PLASMA_FORCE_INLINE T* plDataBlock<T, SizeInBytes>::PopBack()
{
  PLASMA_ASSERT_DEV(m_uiCount > 0, "Block is empty");
  --m_uiCount;
  return m_pData + m_uiCount;
}

template <typename T, plUInt32 SizeInBytes>
PLASMA_ALWAYS_INLINE bool plDataBlock<T, SizeInBytes>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, plUInt32 SizeInBytes>
PLASMA_ALWAYS_INLINE bool plDataBlock<T, SizeInBytes>::IsFull() const
{
  return m_uiCount == CAPACITY;
}

template <typename T, plUInt32 SizeInBytes>
PLASMA_FORCE_INLINE T& plDataBlock<T, SizeInBytes>::operator[](plUInt32 uiIndex) const
{
  PLASMA_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Data block has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return m_pData[uiIndex];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <plUInt32 BlockSize>
plLargeBlockAllocator<BlockSize>::plLargeBlockAllocator(plStringView sName, plAllocatorBase* pParent, plBitflags<plMemoryTrackingFlags> flags)
  : m_TrackingFlags(flags)
  , m_SuperBlocks(pParent)
  , m_FreeBlocks(pParent)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(BlockSize >= 4096, "Block size must be 4096 or bigger");

  m_Id = plMemoryTracker::RegisterAllocator(sName, flags, plPageAllocator::GetId());
  m_ThreadID = plThreadUtils::GetCurrentThreadID();

  const plUInt32 uiPageSize = plSystemInformation::Get().GetMemoryPageSize();
  PLASMA_IGNORE_UNUSED(uiPageSize);
  PLASMA_ASSERT_DEV(uiPageSize <= BlockSize, "Memory Page size is bigger than block size.");
  PLASMA_ASSERT_DEV(BlockSize % uiPageSize == 0, "Blocksize ({0}) must be a multiple of page size ({1})", BlockSize, uiPageSize);
}

template <plUInt32 BlockSize>
plLargeBlockAllocator<BlockSize>::~plLargeBlockAllocator()
{
  PLASMA_ASSERT_RELEASE(m_ThreadID == plThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");
  plMemoryTracker::DeregisterAllocator(m_Id);

  for (plUInt32 i = 0; i < m_SuperBlocks.GetCount(); ++i)
  {
    plPageAllocator::DeallocatePage(m_SuperBlocks[i].m_pBasePtr);
  }
}

template <plUInt32 BlockSize>
template <typename T>
PLASMA_FORCE_INLINE plDataBlock<T, BlockSize> plLargeBlockAllocator<BlockSize>::AllocateBlock()
{
  struct Helper
  {
    enum
    {
      BLOCK_CAPACITY = plDataBlock<T, BlockSize>::CAPACITY
    };
  };

  PLASMA_CHECK_AT_COMPILETIME_MSG(
    Helper::BLOCK_CAPACITY >= 1, "Type is too big for block allocation. Consider using regular heap allocation instead or increase the block size.");

  plDataBlock<T, BlockSize> block(static_cast<T*>(Allocate(PLASMA_ALIGNMENT_OF(T))), 0);
  return block;
}

template <plUInt32 BlockSize>
template <typename T>
PLASMA_FORCE_INLINE void plLargeBlockAllocator<BlockSize>::DeallocateBlock(plDataBlock<T, BlockSize>& inout_block)
{
  Deallocate(inout_block.m_pData);
  inout_block.m_pData = nullptr;
  inout_block.m_uiCount = 0;
}

template <plUInt32 BlockSize>
PLASMA_ALWAYS_INLINE plStringView plLargeBlockAllocator<BlockSize>::GetName() const
{
  return plMemoryTracker::GetAllocatorName(m_Id);
}

template <plUInt32 BlockSize>
PLASMA_ALWAYS_INLINE plAllocatorId plLargeBlockAllocator<BlockSize>::GetId() const
{
  return m_Id;
}

template <plUInt32 BlockSize>
PLASMA_ALWAYS_INLINE const plAllocatorBase::Stats& plLargeBlockAllocator<BlockSize>::GetStats() const
{
  return plMemoryTracker::GetAllocatorStats(m_Id);
}

template <plUInt32 BlockSize>
void* plLargeBlockAllocator<BlockSize>::Allocate(size_t uiAlign)
{
  PLASMA_ASSERT_RELEASE(plMath::IsPowerOf2((plUInt32)uiAlign), "Alignment must be power of two");

  plTime fAllocationTime = plTime::Now();

  PLASMA_LOCK(m_Mutex);

  void* ptr = nullptr;

  if (!m_FreeBlocks.IsEmpty())
  {
    // Re-use a super block
    plUInt32 uiFreeBlockIndex = m_FreeBlocks.PeekBack();
    m_FreeBlocks.PopBack();

    const plUInt32 uiSuperBlockIndex = uiFreeBlockIndex / SuperBlock::NUM_BLOCKS;
    const plUInt32 uiInnerBlockIndex = uiFreeBlockIndex & (SuperBlock::NUM_BLOCKS - 1);
    SuperBlock& superBlock = m_SuperBlocks[uiSuperBlockIndex];
    ++superBlock.m_uiUsedBlocks;

    ptr = plMemoryUtils::AddByteOffset(superBlock.m_pBasePtr, uiInnerBlockIndex * BlockSize);
  }
  else
  {
    // Allocate a new super block
    void* pMemory = plPageAllocator::AllocatePage(SuperBlock::SIZE_IN_BYTES);
    PLASMA_CHECK_ALIGNMENT(pMemory, uiAlign);

    SuperBlock superBlock;
    superBlock.m_pBasePtr = pMemory;
    superBlock.m_uiUsedBlocks = 1;

    m_SuperBlocks.PushBack(superBlock);

    const plUInt32 uiBlockBaseIndex = (m_SuperBlocks.GetCount() - 1) * SuperBlock::NUM_BLOCKS;
    for (plUInt32 i = SuperBlock::NUM_BLOCKS - 1; i > 0; --i)
    {
      m_FreeBlocks.PushBack(uiBlockBaseIndex + i);
    }

    ptr = pMemory;
  }

  if ((m_TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::AddAllocation(m_Id, m_TrackingFlags, ptr, BlockSize, uiAlign, plTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <plUInt32 BlockSize>
void plLargeBlockAllocator<BlockSize>::Deallocate(void* ptr)
{
  PLASMA_LOCK(m_Mutex);

  if ((m_TrackingFlags & plMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    plMemoryTracker::RemoveAllocation(m_Id, ptr);
  }

  // find super block
  bool bFound = false;
  plUInt32 uiSuperBlockIndex = m_SuperBlocks.GetCount();
  std::ptrdiff_t diff = 0;

  for (; uiSuperBlockIndex-- > 0;)
  {
    diff = (char*)ptr - (char*)m_SuperBlocks[uiSuperBlockIndex].m_pBasePtr;
    if (diff >= 0 && diff < SuperBlock::SIZE_IN_BYTES)
    {
      bFound = true;
      break;
    }
  }

  PLASMA_ASSERT_DEV(bFound, "'{0}' was not allocated with this allocator", plArgP(ptr));

  SuperBlock& superBlock = m_SuperBlocks[uiSuperBlockIndex];
  --superBlock.m_uiUsedBlocks;

  if (superBlock.m_uiUsedBlocks == 0 && m_FreeBlocks.GetCount() > SuperBlock::NUM_BLOCKS * 4)
  {
    // give memory back
    plPageAllocator::DeallocatePage(superBlock.m_pBasePtr);

    m_SuperBlocks.RemoveAtAndSwap(uiSuperBlockIndex);
    const plUInt32 uiLastSuperBlockIndex = m_SuperBlocks.GetCount();

    // patch free list
    for (plUInt32 i = 0; i < m_FreeBlocks.GetCount(); ++i)
    {
      const plUInt32 uiIndex = m_FreeBlocks[i];
      const plUInt32 uiSBIndex = uiIndex / SuperBlock::NUM_BLOCKS;

      if (uiSBIndex == uiSuperBlockIndex)
      {
        // points to the block we just removed
        m_FreeBlocks.RemoveAtAndSwap(i);
        --i;
      }
      else if (uiSBIndex == uiLastSuperBlockIndex)
      {
        // points to the block we just swapped
        m_FreeBlocks[i] = uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + (uiIndex & (SuperBlock::NUM_BLOCKS - 1));
      }
    }
  }
  else
  {
    // add block to free list
    const plUInt32 uiInnerBlockIndex = (plUInt32)(diff / BlockSize);
    m_FreeBlocks.PushBack(uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + uiInnerBlockIndex);
  }
}
