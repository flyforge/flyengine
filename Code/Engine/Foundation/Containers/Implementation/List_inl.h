#pragma once

#include <Foundation/Math/Math.h>

// **** ListElement ****

template <typename T>
plListBase<T>::ListElementBase::ListElementBase()
  : m_pPrev(nullptr)
  , m_pNext(nullptr)
{
}

template <typename T>
plListBase<T>::ListElement::ListElement(const T& data)
  : m_Data(data)
{
}

// **** plListBase ****

template <typename T>
plListBase<T>::plListBase(plAllocatorBase* pAllocator)
  : m_End(reinterpret_cast<ListElement*>(&m_Last))
  , m_uiCount(0)
  , m_Elements(pAllocator)
  , m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev = reinterpret_cast<ListElement*>(&m_First);
}

template <typename T>
plListBase<T>::plListBase(const plListBase<T>& cc, plAllocatorBase* pAllocator)
  : m_End(reinterpret_cast<ListElement*>(&m_Last))
  , m_uiCount(0)
  , m_Elements(pAllocator)
  , m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev = reinterpret_cast<ListElement*>(&m_First);

  operator=(cc);
}

template <typename T>
plListBase<T>::~plListBase()
{
  Clear();
}

template <typename T>
void plListBase<T>::operator=(const plListBase<T>& cc)
{
  Clear();
  Insert(GetIterator(), cc.GetIterator(), cc.GetEndIterator());
}

template <typename T>
typename plListBase<T>::ListElement* plListBase<T>::AcquireNode(const T& data)
{
  ListElement* pNode;

  if (m_pFreeElementStack == nullptr)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pNext;
  }

  plMemoryUtils::Construct<ListElement>(pNode, 1);
  pNode->m_Data = data;
  return pNode;
}

template <typename T>
void plListBase<T>::ReleaseNode(ListElement* pNode)
{
  plMemoryUtils::Destruct<ListElement>(pNode, 1);

  if (pNode == &m_Elements.PeekBack())
  {
    m_Elements.PopBack();
  }
  else if (pNode == &m_Elements.PeekFront())
  {
    m_Elements.PopFront();
  }
  else
  {
    pNode->m_pNext = m_pFreeElementStack;
    m_pFreeElementStack = pNode;
  }

  --m_uiCount;
}


template <typename T>
PLASMA_ALWAYS_INLINE typename plListBase<T>::Iterator plListBase<T>::GetIterator()
{
  return Iterator(m_First.m_pNext);
}

template <typename T>
PLASMA_ALWAYS_INLINE typename plListBase<T>::Iterator plListBase<T>::GetLastIterator()
{
  return Iterator(m_Last.m_pPrev);
}

template <typename T>
PLASMA_ALWAYS_INLINE typename plListBase<T>::Iterator plListBase<T>::GetEndIterator()
{
  return m_End;
}

template <typename T>
PLASMA_ALWAYS_INLINE typename plListBase<T>::ConstIterator plListBase<T>::GetIterator() const
{
  return ConstIterator(m_First.m_pNext);
}

template <typename T>
PLASMA_ALWAYS_INLINE typename plListBase<T>::ConstIterator plListBase<T>::GetLastIterator() const
{
  return ConstIterator(m_Last.m_pPrev);
}

template <typename T>
PLASMA_ALWAYS_INLINE typename plListBase<T>::ConstIterator plListBase<T>::GetEndIterator() const
{
  return m_End;
}

template <typename T>
PLASMA_ALWAYS_INLINE plUInt32 plListBase<T>::GetCount() const
{
  return m_uiCount;
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plListBase<T>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename T>
void plListBase<T>::Clear()
{
  if (!IsEmpty())
    Remove(GetIterator(), GetEndIterator());

  m_pFreeElementStack = nullptr;
  m_Elements.Clear();
}

template <typename T>
PLASMA_FORCE_INLINE void plListBase<T>::Compact()
{
  m_Elements.Compact();
}

template <typename T>
PLASMA_FORCE_INLINE T& plListBase<T>::PeekFront()
{
  PLASMA_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
PLASMA_FORCE_INLINE T& plListBase<T>::PeekBack()
{
  PLASMA_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}

template <typename T>
PLASMA_FORCE_INLINE const T& plListBase<T>::PeekFront() const
{
  PLASMA_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
PLASMA_FORCE_INLINE const T& plListBase<T>::PeekBack() const
{
  PLASMA_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}


template <typename T>
PLASMA_ALWAYS_INLINE void plListBase<T>::PushBack()
{
  PushBack(T());
}

template <typename T>
PLASMA_ALWAYS_INLINE void plListBase<T>::PushBack(const T& element)
{
  Insert(GetEndIterator(), element);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plListBase<T>::PushFront()
{
  PushFront(T());
}

template <typename T>
PLASMA_ALWAYS_INLINE void plListBase<T>::PushFront(const T& element)
{
  Insert(GetIterator(), element);
}

template <typename T>
PLASMA_FORCE_INLINE void plListBase<T>::PopBack()
{
  PLASMA_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_Last.m_pPrev));
}

template <typename T>
void plListBase<T>::PopFront()
{
  PLASMA_ASSERT_DEBUG(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_First.m_pNext));
}

template <typename T>
typename plListBase<T>::Iterator plListBase<T>::Insert(const Iterator& pos, const T& data)
{
  PLASMA_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ++m_uiCount;
  ListElement* elem = AcquireNode(data);

  elem->m_pNext = pos.m_pElement;
  elem->m_pPrev = pos.m_pElement->m_pPrev;

  pos.m_pElement->m_pPrev->m_pNext = elem;
  pos.m_pElement->m_pPrev = elem;

  return Iterator(elem);
}

template <typename T>
void plListBase<T>::Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last)
{
  PLASMA_ASSERT_DEV(pos.m_pElement != nullptr && first.m_pElement != nullptr && last.m_pElement != nullptr, "One of the iterators is invalid.");

  while (first != last)
  {
    Insert(pos, *first);
    ++first;
  }
}

template <typename T>
typename plListBase<T>::Iterator plListBase<T>::Remove(const Iterator& pos)
{
  PLASMA_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  PLASMA_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ListElement* pPrev = pos.m_pElement->m_pPrev;
  ListElement* pNext = pos.m_pElement->m_pNext;

  pPrev->m_pNext = pNext;
  pNext->m_pPrev = pPrev;

  ReleaseNode(pos.m_pElement);

  return Iterator(pNext);
}

template <typename T>
typename plListBase<T>::Iterator plListBase<T>::Remove(Iterator first, const Iterator& last)
{
  PLASMA_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  PLASMA_ASSERT_DEV(first.m_pElement != nullptr && last.m_pElement != nullptr, "An iterator is invalid.");

  while (first != last)
    first = Remove(first);

  return last;
}

/*! If uiNewSize is smaller than the size of the list, elements are popped from the back, until the desired size is reached.
    If uiNewSize is larger than the size of the list, default-constructed elements are appended to the list, until the desired size is reached.
*/
template <typename T>
void plListBase<T>::SetCount(plUInt32 uiNewSize)
{
  while (m_uiCount > uiNewSize)
    PopBack();

  while (m_uiCount < uiNewSize)
    PushBack();
}

template <typename T>
bool plListBase<T>::operator==(const plListBase<T>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  auto itLhs = GetIterator();
  auto itRhs = rhs.GetIterator();

  while (itLhs.IsValid())
  {
    if (*itLhs != *itRhs)
      return false;

    ++itLhs;
    ++itRhs;
  }

  return true;
}

template <typename T>
bool plListBase<T>::operator!=(const plListBase<T>& rhs) const
{
  return !operator==(rhs);
}

template <typename T, typename A>
plList<T, A>::plList()
  : plListBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
plList<T, A>::plList(plAllocatorBase* pAllocator)
  : plListBase<T>(pAllocator)
{
}

template <typename T, typename A>
plList<T, A>::plList(const plList<T, A>& other)
  : plListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
plList<T, A>::plList(const plListBase<T>& other)
  : plListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
void plList<T, A>::operator=(const plList<T, A>& rhs)
{
  plListBase<T>::operator=(rhs);
}

template <typename T, typename A>
void plList<T, A>::operator=(const plListBase<T>& rhs)
{
  plListBase<T>::operator=(rhs);
}
