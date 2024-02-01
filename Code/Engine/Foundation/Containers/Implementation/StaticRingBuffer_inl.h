#pragma once

template <typename T, plUInt32 C>
plStaticRingBuffer<T, C>::plStaticRingBuffer()
{
  m_pElements = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount = 0;
}

template <typename T, plUInt32 C>
plStaticRingBuffer<T, C>::plStaticRingBuffer(const plStaticRingBuffer<T, C>& rhs)
{
  m_pElements = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount = 0;

  *this = rhs;
}

template <typename T, plUInt32 C>
plStaticRingBuffer<T, C>::~plStaticRingBuffer()
{
  Clear();
}

template <typename T, plUInt32 C>
void plStaticRingBuffer<T, C>::operator=(const plStaticRingBuffer<T, C>& rhs)
{
  Clear();

  for (plUInt32 i = 0; i < rhs.GetCount(); ++i)
    PushBack(rhs[i]);
}

template <typename T, plUInt32 C>
bool plStaticRingBuffer<T, C>::operator==(const plStaticRingBuffer<T, C>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  for (plUInt32 i = 0; i < m_uiCount; ++i)
  {
    if ((*this)[i] != rhs[i])
      return false;
  }

  return true;
}

template <typename T, plUInt32 C>
void plStaticRingBuffer<T, C>::PushBack(const T& element)
{
  PL_ASSERT_DEV(CanAppend(), "The ring-buffer is full, no elements can be appended before removing one.");

  const plUInt32 uiLastElement = (m_uiFirstElement + m_uiCount) % C;

  plMemoryUtils::CopyConstruct(&m_pElements[uiLastElement], element, 1);
  ++m_uiCount;
}

template <typename T, plUInt32 C>
void plStaticRingBuffer<T, C>::PushBack(T&& element)
{
  PL_ASSERT_DEV(CanAppend(), "The ring-buffer is full, no elements can be appended before removing one.");

  const plUInt32 uiLastElement = (m_uiFirstElement + m_uiCount) % C;

  plMemoryUtils::MoveConstruct(&m_pElements[uiLastElement], std::move(element));
  ++m_uiCount;
}

template <typename T, plUInt32 C>
T& plStaticRingBuffer<T, C>::PeekBack()
{
  PL_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the last element.");

  const plUInt32 uiLastElement = (m_uiFirstElement + m_uiCount - 1) % C;
  return m_pElements[uiLastElement];
}

template <typename T, plUInt32 C>
const T& plStaticRingBuffer<T, C>::PeekBack() const
{
  PL_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the last element.");

  const plUInt32 uiLastElement = (m_uiFirstElement + m_uiCount - 1) % C;
  return m_pElements[uiLastElement];
}

template <typename T, plUInt32 C>
void plStaticRingBuffer<T, C>::PopFront(plUInt32 uiElements)
{
  PL_ASSERT_DEV(m_uiCount >= uiElements, "The ring-buffer contains {0} elements, cannot remove {1} elements from it.", m_uiCount, uiElements);

  while (uiElements > 0)
  {
    plMemoryUtils::Destruct(&m_pElements[m_uiFirstElement], 1);
    ++m_uiFirstElement;
    m_uiFirstElement %= C;
    --m_uiCount;

    --uiElements;
  }
}

template <typename T, plUInt32 C>
PL_FORCE_INLINE const T& plStaticRingBuffer<T, C>::PeekFront() const
{
  PL_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, plUInt32 C>
PL_FORCE_INLINE T& plStaticRingBuffer<T, C>::PeekFront()
{
  PL_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, plUInt32 C>
PL_FORCE_INLINE const T& plStaticRingBuffer<T, C>::operator[](plUInt32 uiIndex) const
{
  PL_ASSERT_DEBUG(uiIndex < m_uiCount, "The ring-buffer only has {0} elements, cannot access element {1}.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, plUInt32 C>
PL_FORCE_INLINE T& plStaticRingBuffer<T, C>::operator[](plUInt32 uiIndex)
{
  PL_ASSERT_DEBUG(uiIndex < m_uiCount, "The ring-buffer only has {0} elements, cannot access element {1}.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, plUInt32 C>
PL_ALWAYS_INLINE plUInt32 plStaticRingBuffer<T, C>::GetCount() const
{
  return m_uiCount;
}

template <typename T, plUInt32 C>
PL_ALWAYS_INLINE bool plStaticRingBuffer<T, C>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, plUInt32 C>
PL_ALWAYS_INLINE bool plStaticRingBuffer<T, C>::CanAppend(plUInt32 uiElements)
{
  return (m_uiCount + uiElements) <= C;
}

template <typename T, plUInt32 C>
void plStaticRingBuffer<T, C>::Clear()
{
  while (!IsEmpty())
    PopFront();
}

template <typename T, plUInt32 C>
PL_ALWAYS_INLINE T* plStaticRingBuffer<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}
