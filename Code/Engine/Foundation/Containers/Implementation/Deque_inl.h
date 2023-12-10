#pragma once

#include <Foundation/Math/Math.h>

#define REDUCE_SIZE(iReduction)     \
  m_iReduceSizeTimer -= iReduction; \
  if (m_iReduceSizeTimer <= 0)      \
    ReduceSize(0);

#define RESERVE(uiCount)                                         \
  if (uiCount > m_uiCount)                                       \
  {                                                              \
    m_uiMaxCount = plMath::Max(m_uiMaxCount, uiCount);           \
    if ((m_uiFirstElement <= 0) || (GetCurMaxCount() < uiCount)) \
      Reserve(uiCount);                                          \
  }

#define CHUNK_SIZE(Type) (4096 / sizeof(Type) < 32 ? 32 : 4096 / sizeof(Type))
//(sizeof(Type) <= 8 ? 256 : (sizeof(Type) <= 16 ? 128 : (sizeof(Type) <= 32 ? 64 : 32))) // although this is Pow(2), this is slower than just having
// larger chunks

template <typename T, bool Construct>
void plDequeBase<T, Construct>::Constructor(plAllocatorBase* pAllocator)
{
  m_pAllocator = pAllocator;
  m_pChunks = nullptr;
  m_uiChunks = 0;
  m_uiFirstElement = 0;
  m_uiCount = 0;
  m_uiAllocatedChunks = 0;
  m_uiMaxCount = 0;

  ResetReduceSizeCounter();

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  m_uiChunkSize = CHUNK_SIZE(T);
#endif
}

template <typename T, bool Construct>
plDequeBase<T, Construct>::plDequeBase(plAllocatorBase* pAllocator)
{
  Constructor(pAllocator);
}

template <typename T, bool Construct>
plDequeBase<T, Construct>::plDequeBase(const plDequeBase<T, Construct>& rhs, plAllocatorBase* pAllocator)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  Constructor(pAllocator);

  *this = rhs;
}

template <typename T, bool Construct>
plDequeBase<T, Construct>::plDequeBase(plDequeBase<T, Construct>&& rhs, plAllocatorBase* pAllocator)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  Constructor(pAllocator);

  *this = std::move(rhs);
}

template <typename T, bool Construct>
plDequeBase<T, Construct>::~plDequeBase()
{
  DeallocateAll();
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::operator=(const plDequeBase<T, Construct>& rhs)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  Clear();                // does not deallocate anything
  RESERVE(rhs.m_uiCount); // allocates data, if required
  m_uiCount = rhs.m_uiCount;

  // copy construct all the elements
  for (plUInt32 i = 0; i < rhs.m_uiCount; ++i)
    plMemoryUtils::CopyConstruct(&ElementAt(i), rhs[i], 1);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::operator=(plDequeBase<T, Construct>&& rhs)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  if (m_pAllocator != rhs.m_pAllocator)
    operator=(static_cast<plDequeBase<T, Construct>&>(rhs));
  else
  {
    DeallocateAll();

    m_uiCount = rhs.m_uiCount;
    m_iReduceSizeTimer = rhs.m_iReduceSizeTimer;
    m_pChunks = rhs.m_pChunks;
    m_uiAllocatedChunks = rhs.m_uiAllocatedChunks;
    m_uiChunks = rhs.m_uiChunks;
    m_uiFirstElement = rhs.m_uiFirstElement;
    m_uiMaxCount = rhs.m_uiMaxCount;

    rhs.m_uiCount = 0;
    rhs.m_pChunks = nullptr;
    rhs.m_uiAllocatedChunks = 0;
    rhs.m_uiChunks = 0;
    rhs.m_uiFirstElement = 0;
    rhs.m_uiMaxCount = 0;
  }
}

template <typename T, bool Construct>
bool plDequeBase<T, Construct>::operator==(const plDequeBase<T, Construct>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  for (plUInt32 i = 0; i < GetCount(); ++i)
  {
    if ((*this)[i] != rhs[i])
      return false;
  }

  return true;
}

template <typename T, bool Construct>
bool plDequeBase<T, Construct>::operator!=(const plDequeBase<T, Construct>& rhs) const
{
  return !operator==(rhs);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::Clear()
{
  if (Construct)
  {
    for (plUInt32 i = 0; i < m_uiCount; ++i)
      plMemoryUtils::Destruct<T>(&operator[](i), 1);
  }

  m_uiCount = 0;

  // since it is much more likely that data is appended at the back of the deque,
  // we do not use the center of the chunk index array, but instead set the first element
  // somewhere more at the front

  // set the first element to a position that allows to add elements at the front
  if (m_uiChunks > 30)
    m_uiFirstElement = CHUNK_SIZE(T) * 16;
  else if (m_uiChunks > 8)
    m_uiFirstElement = CHUNK_SIZE(T) * 4;
  else if (m_uiChunks > 1)
    m_uiFirstElement = CHUNK_SIZE(T) * 1;
  else if (m_uiChunks > 0)
    m_uiFirstElement = 1; // with the current implementation this case should not be possible.
  else
    m_uiFirstElement = 0; // must also work, if Clear is called on a deallocated (not yet allocated) deque
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::Reserve(plUInt32 uiCount)
{
  // This is the function where all the complicated stuff happens.
  // The basic idea is as follows:
  // * do not do anything unless necessary
  // * if the index array (for the redirection) is already large enough to handle the 'address space', try to reuse it
  //   by moving data around (shift it left or right), if necessary
  // * if the chunk index array is not large enough to handle the required amount of redirections, allocate a new
  //   index array and move the old data over
  // This function does not allocate any of the chunks itself (that's what 'ElementAt' does), it only takes care
  // that the amount of reserved elements can be redirected once the deque is enlarged accordingly.

  // no need to change anything in this case
  if (uiCount <= m_uiCount)
    return;

  // keeps track of the largest amount of used elements since the last memory reduction
  m_uiMaxCount = plMath::Max(m_uiMaxCount, uiCount);

  // if there is enough room to hold all requested elements AND one can prepend at least one element (PushFront)
  // do not reallocate
  if ((m_uiFirstElement > 0) && (GetCurMaxCount() >= uiCount))
    return;

  const plUInt32 uiCurFirstChunk = GetFirstUsedChunk();
  const plUInt32 uiRequiredChunks = GetRequiredChunks(uiCount);

  // if we already have enough chunks, just rearrange them
  if (m_uiChunks > uiRequiredChunks + 1) // have at least one spare chunk for the front, and one for the back
  {
    const plUInt32 uiSpareChunks = m_uiChunks - uiRequiredChunks;
    const plUInt32 uiSpareChunksStart = uiSpareChunks / 2;

    PLASMA_ASSERT_DEBUG(uiSpareChunksStart > 0, "Implementation error.");

    // always leave one spare chunk at the front, to ensure that one can prepend elements

    PLASMA_ASSERT_DEBUG(uiSpareChunksStart != uiCurFirstChunk, "No rearrangement possible.");

    // if the new first active chunk is to the left
    if (uiSpareChunksStart < uiCurFirstChunk)
      MoveIndexChunksLeft(uiCurFirstChunk - uiSpareChunksStart);
    else
      MoveIndexChunksRight(uiSpareChunksStart - uiCurFirstChunk);

    PLASMA_ASSERT_DEBUG(m_uiFirstElement > 0, "Did not achieve the desired effect.");
    PLASMA_ASSERT_DEBUG(GetCurMaxCount() >= uiCount, "Did not achieve the desired effect ({0} >= {1}).", GetCurMaxCount(), uiCount);
  }
  else
  {
    const plUInt32 uiReallocSize = 16 + uiRequiredChunks + 16;

    T** pNewChunksArray = PLASMA_NEW_RAW_BUFFER(m_pAllocator, T*, uiReallocSize);
    plMemoryUtils::ZeroFill(pNewChunksArray, uiReallocSize);

    const plUInt32 uiFirstUsedChunk = m_uiFirstElement / CHUNK_SIZE(T);

    // move all old chunks over
    plUInt32 pos = 16;

    // first the used chunks at the start of the new array
    for (plUInt32 i = 0; i < m_uiChunks - uiFirstUsedChunk; ++i)
    {
      pNewChunksArray[pos] = m_pChunks[uiFirstUsedChunk + i];
      ++pos;
    }

    m_uiFirstElement -= uiFirstUsedChunk * CHUNK_SIZE(T);

    // then the unused chunks at the end of the new array
    for (plUInt32 i = 0; i < uiFirstUsedChunk; ++i)
    {
      pNewChunksArray[pos] = m_pChunks[i];
      ++pos;
    }

    m_uiFirstElement += 16 * CHUNK_SIZE(T);

    PLASMA_ASSERT_DEBUG(m_uiFirstElement == (16 * CHUNK_SIZE(T)) + (m_uiFirstElement % CHUNK_SIZE(T)), "");


    PLASMA_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);
    m_pChunks = pNewChunksArray;
    m_uiChunks = uiReallocSize;
  }
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::Compact()
{
  ResetReduceSizeCounter();

  if (IsEmpty())
  {
    DeallocateAll();
    return;
  }

  // this will deallocate ALL unused chunks
  DeallocateUnusedChunks(GetRequiredChunks(m_uiCount));

  // reduces the size of the index array, but keeps some spare pointers, so that scaling up is still possible without reallocation
  CompactIndexArray(0);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::Swap(plDequeBase<T, Construct>& other)
{
  plMath::Swap(this->m_pAllocator, other.m_pAllocator);
  plMath::Swap(this->m_pChunks, other.m_pChunks);
  plMath::Swap(this->m_uiChunks, other.m_uiChunks);
  plMath::Swap(this->m_uiFirstElement, other.m_uiFirstElement);
  plMath::Swap(this->m_uiCount, other.m_uiCount);
  plMath::Swap(this->m_uiAllocatedChunks, other.m_uiAllocatedChunks);
  plMath::Swap(this->m_iReduceSizeTimer, other.m_iReduceSizeTimer);
  plMath::Swap(this->m_uiMaxCount, other.m_uiMaxCount);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::CompactIndexArray(plUInt32 uiMinChunksToKeep)
{
  const plUInt32 uiRequiredChunks = plMath::Max<plUInt32>(1, GetRequiredChunks(m_uiCount));
  uiMinChunksToKeep = plMath::Max(uiRequiredChunks, uiMinChunksToKeep);

  // keep some spare pointers for scaling the deque up again
  const plUInt32 uiChunksToKeep = 16 + uiMinChunksToKeep + 16;

  // only reduce the index array, if we can reduce its size at least to half (the +4 is for the very small cases)
  if (uiChunksToKeep + 4 >= m_uiChunks / 2)
    return;

  T** pNewChunkArray = PLASMA_NEW_RAW_BUFFER(m_pAllocator, T*, uiChunksToKeep);
  plMemoryUtils::ZeroFill<T*>(pNewChunkArray, uiChunksToKeep);

  const plUInt32 uiFirstChunk = GetFirstUsedChunk();

  // makes sure that no more than this amount of chunks is still allocated -> those can be copied over
  DeallocateUnusedChunks(uiChunksToKeep);

  // moves the used chunks into the new array
  for (plUInt32 i = 0; i < uiRequiredChunks; ++i)
  {
    pNewChunkArray[16 + i] = m_pChunks[uiFirstChunk + i];
    m_pChunks[uiFirstChunk + i] = nullptr;
  }

  // copy all still allocated chunks over to the new index array
  // since we just deallocated enough chunks, all that are found can be copied over as spare chunks
  {
    plUInt32 iPos = 0;
    for (plUInt32 i = 0; i < uiFirstChunk; ++i)
    {
      if (m_pChunks[i])
      {
        PLASMA_ASSERT_DEBUG(iPos < 16 || ((iPos >= 16 + uiRequiredChunks) && (iPos < uiChunksToKeep)), "Implementation error.");

        pNewChunkArray[iPos] = m_pChunks[i];
        m_pChunks[i] = nullptr;
        ++iPos;

        if (iPos == 16)
          iPos += uiRequiredChunks;
      }
    }

    for (plUInt32 i = GetLastUsedChunk() + 1; i < m_uiChunks; ++i)
    {
      if (m_pChunks[i])
      {
        PLASMA_ASSERT_DEBUG(iPos < 16 || ((iPos >= 16 + uiRequiredChunks) && (iPos < uiChunksToKeep)), "Implementation error.");

        pNewChunkArray[iPos] = m_pChunks[i];
        m_pChunks[i] = nullptr;
        ++iPos;

        if (iPos == 16)
          iPos += uiRequiredChunks;
      }
    }
  }

  PLASMA_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);
  m_pChunks = pNewChunkArray;
  m_uiChunks = uiChunksToKeep;
  m_uiFirstElement = (16 * CHUNK_SIZE(T)) + (m_uiFirstElement % CHUNK_SIZE(T));
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::SetCount(plUInt32 uiCount)
{
  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    // grow the deque

    RESERVE(uiNewCount);
    m_uiCount = uiNewCount;

    if (Construct)
    {
      // default construct the new elements
      for (plUInt32 i = uiOldCount; i < uiNewCount; ++i)
        plMemoryUtils::DefaultConstruct(&ElementAt(i), 1);
    }
    else
    {
      for (plUInt32 i = uiOldCount; i < uiNewCount; ++i)
        ElementAt(i);
    }
  }
  else
  {
    if (Construct)
    {
      // destruct elements at the end of the deque
      for (plUInt32 i = uiNewCount; i < uiOldCount; ++i)
        plMemoryUtils::Destruct(&operator[](i), 1);
    }

    m_uiCount = uiNewCount;

    // if enough elements have been destructed, trigger a size reduction (the first time will not deallocate anything though)
    ReduceSize(uiOldCount - uiNewCount);
  }
}

template <typename T, bool Construct>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger
// early.
void plDequeBase<T, Construct>::SetCountUninitialized(plUInt32 uiCount)
{
  static_assert(plIsPodType<T>::value == plTypeIsPod::value, "SetCountUninitialized is only supported for POD types.");

  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    // grow the deque

    RESERVE(uiNewCount);
    m_uiCount = uiNewCount;

    for (plUInt32 i = uiOldCount; i < uiNewCount; ++i)
      ElementAt(i);
  }
  else
  {
    if (Construct)
    {
      // destruct elements at the end of the deque
      for (plUInt32 i = uiNewCount; i < uiOldCount; ++i)
        plMemoryUtils::Destruct(&operator[](i), 1);
    }

    m_uiCount = uiNewCount;

    // if enough elements have been destructed, trigger a size reduction (the first time will not deallocate anything though)
    ReduceSize(uiOldCount - uiNewCount);
  }
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::EnsureCount(plUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, bool Construct>
inline plUInt32 plDequeBase<T, Construct>::GetContiguousRange(plUInt32 uiIndex) const
{
  PLASMA_ASSERT_DEV(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const plUInt32 uiChunkSize = CHUNK_SIZE(T);

  const plUInt32 uiRealIndex = m_uiFirstElement + uiIndex;
  const plUInt32 uiChunkOffset = uiRealIndex % uiChunkSize;

  const plUInt32 uiRange = uiChunkSize - uiChunkOffset;

  return plMath::Min(uiRange, GetCount() - uiIndex);
}

template <typename T, bool Construct>
inline T& plDequeBase<T, Construct>::operator[](plUInt32 uiIndex)
{
  PLASMA_ASSERT_DEBUG(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const plUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const plUInt32 uiChunkIndex = uiRealIndex / CHUNK_SIZE(T);
  const plUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
inline const T& plDequeBase<T, Construct>::operator[](plUInt32 uiIndex) const
{
  PLASMA_ASSERT_DEBUG(uiIndex < m_uiCount, "The deque has {0} elements. Cannot access element {1}.", m_uiCount, uiIndex);

  const plUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const plUInt32 uiChunkIndex = uiRealIndex / CHUNK_SIZE(T);
  const plUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
inline T& plDequeBase<T, Construct>::ExpandAndGetRef()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  T* pElement = &ElementAt(m_uiCount - 1);

  if (Construct)
    plMemoryUtils::DefaultConstruct(pElement, 1);

  return *pElement;
}

template <typename T, bool Construct>
inline void plDequeBase<T, Construct>::PushBack()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  T* pElement = &ElementAt(m_uiCount - 1);

  if (Construct)
    plMemoryUtils::DefaultConstruct(pElement, 1);
}

template <typename T, bool Construct>
inline void plDequeBase<T, Construct>::PushBack(const T& element)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  plMemoryUtils::CopyConstruct(&ElementAt(m_uiCount - 1), element, 1);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::PushBack(T&& element)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;

  plMemoryUtils::MoveConstruct<T>(&ElementAt(m_uiCount - 1), std::move(element));
}

template <typename T, bool Construct>
inline void plDequeBase<T, Construct>::PopBack(plUInt32 uiElements)
{
  PLASMA_ASSERT_DEV(uiElements <= GetCount(), "Cannot remove {0} elements, the deque only contains {1} elements.", uiElements, GetCount());

  for (plUInt32 i = 0; i < uiElements; ++i)
  {
    if (Construct)
      plMemoryUtils::Destruct(&operator[](m_uiCount - 1), 1);

    --m_uiCount;
  }

  // might trigger a memory reduction
  REDUCE_SIZE(uiElements);
}

template <typename T, bool Construct>
inline void plDequeBase<T, Construct>::PushFront(const T& element)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  plMemoryUtils::CopyConstruct(&ElementAt(0), element, 1);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::PushFront(T&& element)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  plMemoryUtils::MoveConstruct<T>(&ElementAt(0), std::move(element));
}

template <typename T, bool Construct>
inline void plDequeBase<T, Construct>::PushFront()
{
  RESERVE(m_uiCount + 1);
  ++m_uiCount;
  --m_uiFirstElement;

  T* pElement = &ElementAt(0);

  if (Construct)
    plMemoryUtils::Construct(pElement, 1);
}

template <typename T, bool Construct>
inline void plDequeBase<T, Construct>::PopFront(plUInt32 uiElements)
{
  PLASMA_ASSERT_DEV(uiElements <= GetCount(), "Cannot remove {0} elements, the deque only contains {1} elements.", uiElements, GetCount());

  for (plUInt32 i = 0; i < uiElements; ++i)
  {
    if (Construct)
      plMemoryUtils::Destruct(&operator[](0), 1);

    --m_uiCount;
    ++m_uiFirstElement;
  }

  // might trigger a memory reduction
  REDUCE_SIZE(uiElements);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE bool plDequeBase<T, Construct>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE plUInt32 plDequeBase<T, Construct>::GetCount() const
{
  return m_uiCount;
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE const T& plDequeBase<T, Construct>::PeekFront() const
{
  return operator[](0);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE T& plDequeBase<T, Construct>::PeekFront()
{
  return operator[](0);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE const T& plDequeBase<T, Construct>::PeekBack() const
{
  return operator[](m_uiCount - 1);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE T& plDequeBase<T, Construct>::PeekBack()
{
  return operator[](m_uiCount - 1);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE bool plDequeBase<T, Construct>::Contains(const T& value) const
{
  return IndexOf(value) != plInvalidIndex;
}

template <typename T, bool Construct>
plUInt32 plDequeBase<T, Construct>::IndexOf(const T& value, plUInt32 uiStartIndex) const
{
  for (plUInt32 i = uiStartIndex; i < m_uiCount; ++i)
  {
    if (plMemoryUtils::IsEqual(&operator[](i), &value))
      return i;
  }

  return plInvalidIndex;
}

template <typename T, bool Construct>
plUInt32 plDequeBase<T, Construct>::LastIndexOf(const T& value, plUInt32 uiStartIndex) const
{
  for (plUInt32 i = plMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (plMemoryUtils::IsEqual(&operator[](i), &value))
      return i;
  }
  return plInvalidIndex;
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::RemoveAtAndSwap(plUInt32 uiIndex)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  PLASMA_ASSERT_DEV(uiIndex < m_uiCount, "Cannot remove element {0}, the deque only contains {1} elements.", uiIndex, m_uiCount);

  if (uiIndex + 1 < m_uiCount) // do not copy over the same element, if uiIndex is actually the last element
    operator[](uiIndex) = PeekBack();

  PopBack();
}

template <typename T, bool Construct>
PLASMA_FORCE_INLINE void plDequeBase<T, Construct>::MoveIndexChunksLeft(plUInt32 uiChunkDiff)
{
  const plUInt32 uiCurFirstChunk = GetFirstUsedChunk();
  const plUInt32 uiRemainingChunks = m_uiChunks - uiCurFirstChunk;
  const plUInt32 uiNewFirstChunk = uiCurFirstChunk - uiChunkDiff;

  // ripple the chunks from the back to the front (in place)
  for (plUInt32 front = 0; front < uiRemainingChunks; ++front)
    plMath::Swap(m_pChunks[uiNewFirstChunk + front], m_pChunks[front + uiCurFirstChunk]);

  // just ensures that the following subtraction is possible
  PLASMA_ASSERT_DEBUG(m_uiFirstElement > uiChunkDiff * CHUNK_SIZE(T), "");

  // adjust which element is the first by how much the index array has been moved
  m_uiFirstElement -= uiChunkDiff * CHUNK_SIZE(T);
}

template <typename T, bool Construct>
PLASMA_FORCE_INLINE void plDequeBase<T, Construct>::MoveIndexChunksRight(plUInt32 uiChunkDiff)
{
  const plUInt32 uiCurFirstChunk = GetFirstUsedChunk();
  const plUInt32 uiLastChunk = (m_uiCount == 0) ? (m_uiFirstElement / CHUNK_SIZE(T)) : ((m_uiFirstElement + m_uiCount - 1) / CHUNK_SIZE(T));
  const plUInt32 uiCopyChunks = (uiLastChunk - uiCurFirstChunk) + 1;

  // ripple the chunks from the front to the back (in place)
  for (plUInt32 i = 0; i < uiCopyChunks; ++i)
    plMath::Swap(m_pChunks[uiLastChunk - i], m_pChunks[uiLastChunk + uiChunkDiff - i]);

  // adjust which element is the first by how much the index array has been moved
  m_uiFirstElement += uiChunkDiff * CHUNK_SIZE(T);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE plUInt32 plDequeBase<T, Construct>::GetFirstUsedChunk() const
{
  return m_uiFirstElement / CHUNK_SIZE(T);
}

template <typename T, bool Construct>
PLASMA_FORCE_INLINE plUInt32 plDequeBase<T, Construct>::GetLastUsedChunk(plUInt32 uiAtSize) const
{
  if (uiAtSize == 0)
    return GetFirstUsedChunk();

  return (m_uiFirstElement + uiAtSize - 1) / CHUNK_SIZE(T);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE plUInt32 plDequeBase<T, Construct>::GetLastUsedChunk() const
{
  return GetLastUsedChunk(m_uiCount);
}

template <typename T, bool Construct>
PLASMA_FORCE_INLINE plUInt32 plDequeBase<T, Construct>::GetRequiredChunks(plUInt32 uiAtSize) const
{
  if (uiAtSize == 0)
    return 0;

  return GetLastUsedChunk(uiAtSize) - GetFirstUsedChunk() + 1;
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::DeallocateUnusedChunks(plUInt32 uiMaxChunks)
{
  if (m_uiAllocatedChunks <= uiMaxChunks)
    return;

  // check all unused chunks at the end, deallocate all that are allocated
  for (plUInt32 i = GetLastUsedChunk() + 1; i < m_uiChunks; ++i)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      PLASMA_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);

      if (m_uiAllocatedChunks <= uiMaxChunks)
        return;
    }
  }

  // check all unused chunks at the front, deallocate all that are allocated
  const plUInt32 uiFirstChunk = GetFirstUsedChunk();

  for (plUInt32 i = 0; i < uiFirstChunk; ++i)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      PLASMA_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);

      if (m_uiAllocatedChunks <= uiMaxChunks)
        return;
    }
  }
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE void plDequeBase<T, Construct>::ResetReduceSizeCounter()
{
  m_iReduceSizeTimer = CHUNK_SIZE(T) * 8; // every time 8 chunks might be unused -> check whether to reduce the deque's size
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::ReduceSize(plInt32 iReduction)
{
  m_iReduceSizeTimer -= iReduction;

  // only trigger the size reduction every once in a while (after enough size reduction that actually a few chunks might be unused)
  if (m_iReduceSizeTimer > 0)
    return;

  ResetReduceSizeCounter();

  // we keep this amount of chunks
  // m_uiMaxCount will be adjusted over time
  // if the deque is shrunk and operates in this state long enough, m_uiMaxCount will be reduced more and more
  const plUInt32 uiMaxChunks = (m_uiMaxCount / CHUNK_SIZE(T)) + 3; // +1 because of rounding, +2 spare chunks

  PLASMA_ASSERT_DEBUG(uiMaxChunks >= GetRequiredChunks(m_uiCount), "Implementation Error.");

  DeallocateUnusedChunks(uiMaxChunks);

  // lerp between the current MaxCount and the actually active number of elements
  // m_uiMaxCount is never smaller than m_uiCount, but m_uiCount might be smaller
  // thus m_uiMaxCount might be reduced over time
  m_uiMaxCount = plMath::Max(m_uiCount, (m_uiMaxCount / 2) + (m_uiCount / 2));

  // Should we really adjust the size of the index array here?
  CompactIndexArray(uiMaxChunks);
}

template <typename T, bool Construct>
PLASMA_ALWAYS_INLINE plUInt32 plDequeBase<T, Construct>::GetCurMaxCount() const
{
  return m_uiChunks * CHUNK_SIZE(T) - m_uiFirstElement;
}

template <typename T, bool Construct>
PLASMA_FORCE_INLINE T* plDequeBase<T, Construct>::GetUnusedChunk()
{
  // first search for an unused, but already allocated, chunk and reuse it, if possible
  const plUInt32 uiCurFirstChunk = GetFirstUsedChunk();

  // search the unused blocks at the start
  for (plUInt32 i = 0; i < uiCurFirstChunk; ++i)
  {
    if (m_pChunks[i])
    {
      T* pChunk = m_pChunks[i];
      m_pChunks[i] = nullptr;
      return pChunk;
    }
  }

  const plUInt32 uiCurLastChunk = GetLastUsedChunk();

  // search the unused blocks at the end
  for (plUInt32 i = m_uiChunks - 1; i > uiCurLastChunk; --i)
  {
    if (m_pChunks[i])
    {
      T* pChunk = m_pChunks[i];
      m_pChunks[i] = nullptr;
      return pChunk;
    }
  }

  // nothing unused found, allocate a new block
  ResetReduceSizeCounter();
  ++m_uiAllocatedChunks;
  return PLASMA_NEW_RAW_BUFFER(m_pAllocator, T, CHUNK_SIZE(T));
}

template <typename T, bool Construct>
T& plDequeBase<T, Construct>::ElementAt(plUInt32 uiIndex)
{
  PLASMA_ASSERT_DEBUG(uiIndex < m_uiCount, "");

  const plUInt32 uiRealIndex = m_uiFirstElement + uiIndex;

  const plUInt32 uiChunkIndex = uiRealIndex / CHUNK_SIZE(T);
  const plUInt32 uiChunkOffset = uiRealIndex % CHUNK_SIZE(T);

  PLASMA_ASSERT_DEBUG(uiChunkIndex < m_uiChunks, "");

  if (m_pChunks[uiChunkIndex] == nullptr)
    m_pChunks[uiChunkIndex] = GetUnusedChunk();

  return m_pChunks[uiChunkIndex][uiChunkOffset];
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::DeallocateAll()
{
  Clear();

  plUInt32 i = 0;
  while (m_uiAllocatedChunks > 0)
  {
    if (m_pChunks[i])
    {
      --m_uiAllocatedChunks;
      PLASMA_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks[i]);
    }

    ++i;
  }

  PLASMA_DELETE_RAW_BUFFER(m_pAllocator, m_pChunks);

  Constructor(m_pAllocator);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::RemoveAtAndCopy(plUInt32 uiIndex)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  PLASMA_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex);

  for (plUInt32 i = uiIndex + 1; i < m_uiCount; ++i)
  {
    plMemoryUtils::CopyOverlapped(&operator[](i - 1), &operator[](i), 1);
  }

  PopBack();
}

template <typename T, bool Construct>
bool plDequeBase<T, Construct>::RemoveAndCopy(const T& value)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  plUInt32 uiIndex = IndexOf(value);

  if (uiIndex == plInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, bool Construct>
bool plDequeBase<T, Construct>::RemoveAndSwap(const T& value)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  plUInt32 uiIndex = IndexOf(value);

  if (uiIndex == plInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::Insert(const T& value, plUInt32 uiIndex)
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(Construct, "This function is not supported on Deques that do not construct their data.");

  // Index 0 inserts before the first element, Index m_uiCount inserts after the last element.
  PLASMA_ASSERT_DEV(uiIndex <= m_uiCount, "The deque has {0} elements. Cannot insert an element at index {1}.", m_uiCount, uiIndex);

  PushBack();

  for (plUInt32 i = m_uiCount - 1; i > uiIndex; --i)
  {
    plMemoryUtils::Copy(&operator[](i), &operator[](i - 1), 1);
  }

  plMemoryUtils::Copy(&operator[](uiIndex), &value, 1);
}

template <typename T, bool Construct>
template <typename Comparer>
void plDequeBase<T, Construct>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
    plSorting::QuickSort(*this, comparer);
}

template <typename T, bool Construct>
void plDequeBase<T, Construct>::Sort()
{
  if (m_uiCount > 1)
    plSorting::QuickSort(*this, plCompareHelper<T>());
}

template <typename T, bool Construct>
plUInt64 plDequeBase<T, Construct>::GetHeapMemoryUsage() const
{
  if (m_pChunks == nullptr)
    return 0;

  plUInt64 res = m_uiChunks * sizeof(T*);

  for (plUInt32 i = 0; i < m_uiChunks; ++i)
  {
    if (m_pChunks[i] != nullptr)
    {
      res += (plUInt64)(CHUNK_SIZE(T)) * (plUInt64)sizeof(T);
    }
  }

  return res;
}

#undef REDUCE_SIZE
#undef RESERVE


template <typename T, typename A, bool Construct>
plDeque<T, A, Construct>::plDeque()
  : plDequeBase<T, Construct>(A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
plDeque<T, A, Construct>::plDeque(plAllocatorBase* pAllocator)
  : plDequeBase<T, Construct>(pAllocator)
{
}

template <typename T, typename A, bool Construct>
plDeque<T, A, Construct>::plDeque(const plDeque<T, A, Construct>& other)
  : plDequeBase<T, Construct>(other, A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
plDeque<T, A, Construct>::plDeque(plDeque<T, A, Construct>&& other)
  : plDequeBase<T, Construct>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A, bool Construct>
plDeque<T, A, Construct>::plDeque(const plDequeBase<T, Construct>& other)
  : plDequeBase<T, Construct>(other, A::GetAllocator())
{
}

template <typename T, typename A, bool Construct>
plDeque<T, A, Construct>::plDeque(plDequeBase<T, Construct>&& other)
  : plDequeBase<T, Construct>(std::move(other), other.GetAllocator())
{
}

template <typename T, typename A, bool Construct>
void plDeque<T, A, Construct>::operator=(const plDeque<T, A, Construct>& rhs)
{
  plDequeBase<T, Construct>::operator=(rhs);
}

template <typename T, typename A, bool Construct>
void plDeque<T, A, Construct>::operator=(plDeque<T, A, Construct>&& rhs)
{
  plDequeBase<T, Construct>::operator=(std::move(rhs));
}

template <typename T, typename A, bool Construct>
void plDeque<T, A, Construct>::operator=(const plDequeBase<T, Construct>& rhs)
{
  plDequeBase<T, Construct>::operator=(rhs);
}

template <typename T, typename A, bool Construct>
void plDeque<T, A, Construct>::operator=(plDequeBase<T, Construct>&& rhs)
{
  plDequeBase<T, Construct>::operator=(std::move(rhs));
}
