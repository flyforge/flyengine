
template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE plBlockStorage<T, BlockSize, StorageType>::ConstIterator::ConstIterator(
  const plBlockStorage<T, BlockSize, StorageType>& storage, plUInt32 uiStartIndex, plUInt32 uiCount)
  : m_Storage(storage)
{
  m_uiCurrentIndex = uiStartIndex;
  m_uiEndIndex = plMath::Max(uiStartIndex + uiCount, uiCount);

  if (StorageType == plBlockStorageType::FreeList)
  {
    plUInt32 uiEndIndex = plMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsBitSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE T& plBlockStorage<T, BlockSize, StorageType>::ConstIterator::CurrentElement() const
{
  const plUInt32 uiBlockIndex = m_uiCurrentIndex / plDataBlock<T, BlockSize>::CAPACITY;
  const plUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * plDataBlock<T, BlockSize>::CAPACITY;
  return m_Storage.m_Blocks[uiBlockIndex][uiInnerIndex];
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE const T& plBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator*() const
{
  return CurrentElement();
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE const T* plBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator->() const
{
  return &CurrentElement();
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE plBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator const T*() const
{
  return &CurrentElement();
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE void plBlockStorage<T, BlockSize, StorageType>::ConstIterator::Next()
{
  ++m_uiCurrentIndex;

  if (StorageType == plBlockStorageType::FreeList)
  {
    plUInt32 uiEndIndex = plMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsBitSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE bool plBlockStorage<T, BlockSize, StorageType>::ConstIterator::IsValid() const
{
  return m_uiCurrentIndex < plMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE void plBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator++()
{
  Next();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE plBlockStorage<T, BlockSize, StorageType>::Iterator::Iterator(
  const plBlockStorage<T, BlockSize, StorageType>& storage, plUInt32 uiStartIndex, plUInt32 uiCount)
  : ConstIterator(storage, uiStartIndex, uiCount)
{
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE T& plBlockStorage<T, BlockSize, StorageType>::Iterator::operator*()
{
  return this->CurrentElement();
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE T* plBlockStorage<T, BlockSize, StorageType>::Iterator::operator->()
{
  return &(this->CurrentElement());
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE plBlockStorage<T, BlockSize, StorageType>::Iterator::operator T*()
{
  return &(this->CurrentElement());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE plBlockStorage<T, BlockSize, StorageType>::plBlockStorage(
  plLargeBlockAllocator<BlockSize>* pBlockAllocator, plAllocator* pAllocator)
  : m_pBlockAllocator(pBlockAllocator)
  , m_Blocks(pAllocator)

{
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
plBlockStorage<T, BlockSize, StorageType>::~plBlockStorage()
{
  Clear();
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
void plBlockStorage<T, BlockSize, StorageType>::Clear()
{
  for (plUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    plDataBlock<T, BlockSize>& block = m_Blocks[uiBlockIndex];

    if (StorageType == plBlockStorageType::Compact)
    {
      plMemoryUtils::Destruct(block.m_pData, block.m_uiCount);
    }
    else
    {
      for (plUInt32 uiInnerIndex = 0; uiInnerIndex < block.m_uiCount; ++uiInnerIndex)
      {
        plUInt32 uiIndex = uiBlockIndex * plDataBlock<T, BlockSize>::CAPACITY + uiInnerIndex;
        if (m_UsedEntries.IsBitSet(uiIndex))
        {
          plMemoryUtils::Destruct(&block.m_pData[uiInnerIndex], 1);
        }
      }
    }

    m_pBlockAllocator->DeallocateBlock(block);
  }

  m_Blocks.Clear();
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
T* plBlockStorage<T, BlockSize, StorageType>::Create()
{
  T* pNewObject = nullptr;
  plUInt32 uiNewIndex = plInvalidIndex;

  if (StorageType == plBlockStorageType::FreeList && m_uiFreelistStart != plInvalidIndex)
  {
    uiNewIndex = m_uiFreelistStart;

    const plUInt32 uiBlockIndex = uiNewIndex / plDataBlock<T, BlockSize>::CAPACITY;
    const plUInt32 uiInnerIndex = uiNewIndex - uiBlockIndex * plDataBlock<T, BlockSize>::CAPACITY;

    pNewObject = &(m_Blocks[uiBlockIndex][uiInnerIndex]);

    m_uiFreelistStart = *reinterpret_cast<plUInt32*>(pNewObject);
  }
  else
  {
    plDataBlock<T, BlockSize>* pBlock = nullptr;

    if (m_Blocks.GetCount() > 0)
    {
      pBlock = &m_Blocks.PeekBack();
    }

    if (pBlock == nullptr || pBlock->IsFull())
    {
      m_Blocks.PushBack(m_pBlockAllocator->template AllocateBlock<T>());
      pBlock = &m_Blocks.PeekBack();
    }

    pNewObject = pBlock->ReserveBack();
    uiNewIndex = m_uiCount;

    ++m_uiCount;
  }

  plMemoryUtils::Construct<SkipTrivialTypes>(pNewObject, 1);

  if (StorageType == plBlockStorageType::FreeList)
  {
    m_UsedEntries.SetCount(m_uiCount);
    m_UsedEntries.SetBit(uiNewIndex);
  }

  return pNewObject;
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE void plBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject)
{
  T* pDummy;
  Delete(pObject, pDummy);
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
void plBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject)
{
  Delete(pObject, out_pMovedObject, plTraitInt<StorageType>());
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE plUInt32 plBlockStorage<T, BlockSize, StorageType>::GetCount() const
{
  return m_uiCount;
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE typename plBlockStorage<T, BlockSize, StorageType>::Iterator plBlockStorage<T, BlockSize, StorageType>::GetIterator(
  plUInt32 uiStartIndex /*= 0*/, plUInt32 uiCount /*= plInvalidIndex*/)
{
  return Iterator(*this, uiStartIndex, uiCount);
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_ALWAYS_INLINE typename plBlockStorage<T, BlockSize, StorageType>::ConstIterator plBlockStorage<T, BlockSize, StorageType>::GetIterator(
  plUInt32 uiStartIndex /*= 0*/, plUInt32 uiCount /*= plInvalidIndex*/) const
{
  return ConstIterator(*this, uiStartIndex, uiCount);
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE void plBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, plTraitInt<plBlockStorageType::Compact>)
{
  plDataBlock<T, BlockSize>& lastBlock = m_Blocks.PeekBack();
  T* pLast = lastBlock.PopBack();

  --m_uiCount;
  if (pObject != pLast)
  {
    plMemoryUtils::Relocate(pObject, pLast, 1);
  }
  else
  {
    plMemoryUtils::Destruct(pLast, 1);
  }

  out_pMovedObject = pLast;

  if (lastBlock.IsEmpty())
  {
    m_pBlockAllocator->DeallocateBlock(lastBlock);
    m_Blocks.PopBack();
  }
}

template <typename T, plUInt32 BlockSize, plBlockStorageType::Enum StorageType>
PL_FORCE_INLINE void plBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, plTraitInt<plBlockStorageType::FreeList>)
{
  plUInt32 uiIndex = plInvalidIndex;
  for (plUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    std::ptrdiff_t diff = pObject - m_Blocks[uiBlockIndex].m_pData;
    if (diff >= 0 && diff < plDataBlock<T, BlockSize>::CAPACITY)
    {
      uiIndex = uiBlockIndex * plDataBlock<T, BlockSize>::CAPACITY + (plInt32)diff;
      break;
    }
  }

  PL_ASSERT_DEV(uiIndex != plInvalidIndex, "Invalid object {0} was not found in block storage.", plArgP(pObject));

  m_UsedEntries.ClearBit(uiIndex);

  out_pMovedObject = pObject;
  plMemoryUtils::Destruct(pObject, 1);

  *reinterpret_cast<plUInt32*>(pObject) = m_uiFreelistStart;
  m_uiFreelistStart = uiIndex;
}
