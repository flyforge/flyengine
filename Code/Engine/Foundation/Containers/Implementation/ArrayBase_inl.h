
template <typename T, typename Derived>
plArrayBase<T, Derived>::plArrayBase() = default;

template <typename T, typename Derived>
plArrayBase<T, Derived>::~plArrayBase()
{
  PLASMA_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  PLASMA_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::operator=(const plArrayPtr<const T>& rhs)
{
  if (this->GetData() == rhs.GetPtr())
  {
    if (m_uiCount == rhs.GetCount())
      return;

    PLASMA_ASSERT_DEV(m_uiCount > rhs.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    plMemoryUtils::Destruct(pElements + rhs.GetCount(), m_uiCount - rhs.GetCount());
    m_uiCount = rhs.GetCount();
    return;
  }

  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = rhs.GetCount();

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    plMemoryUtils::Copy(pElements, rhs.GetPtr(), uiOldCount);
    plMemoryUtils::CopyConstructArray(pElements + uiOldCount, rhs.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = static_cast<Derived*>(this)->GetElementsPtr();
    plMemoryUtils::Copy(pElements, rhs.GetPtr(), uiNewCount);
    plMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiNewCount;
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE plArrayBase<T, Derived>::operator plArrayPtr<const T>() const
{
  return plArrayPtr<const T>(static_cast<const Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE plArrayBase<T, Derived>::operator plArrayPtr<T>()
{
  return plArrayPtr<T>(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
}

template <typename T, typename Derived>
bool plArrayBase<T, Derived>::operator==(const plArrayPtr<const T>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return plMemoryUtils::IsEqual(static_cast<const Derived*>(this)->GetElementsPtr(), rhs.GetPtr(), m_uiCount);
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE bool plArrayBase<T, Derived>::operator!=(const plArrayPtr<const T>& rhs) const
{
  return !(*this == rhs);
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE bool plArrayBase<T, Derived>::operator<(const plArrayPtr<const T>& rhs) const
{
  return GetArrayPtr() < rhs;
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE const T& plArrayBase<T, Derived>::operator[](const plUInt32 uiIndex) const
{
  PLASMA_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return static_cast<const Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE T& plArrayBase<T, Derived>::operator[](const plUInt32 uiIndex)
{
  PLASMA_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return static_cast<Derived*>(this)->GetElementsPtr()[uiIndex];
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::SetCount(plUInt32 uiCount)
{
  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    plMemoryUtils::DefaultConstruct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    plMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::SetCount(plUInt32 uiCount, const T& fillValue)
{
  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    plMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, fillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    plMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::EnsureCount(plUInt32 uiCount)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount);
  }
}

template <typename T, typename Derived>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger
// early.
void plArrayBase<T, Derived>::SetCountUninitialized(plUInt32 uiCount)
{
  static_assert(plIsPodType<T>::value == plTypeIsPod::value, "SetCountUninitialized is only supported for POD types.");
  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    static_cast<Derived*>(this)->Reserve(uiNewCount);
    plMemoryUtils::Construct(static_cast<Derived*>(this)->GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    plMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE plUInt32 plArrayBase<T, Derived>::GetCount() const
{
  return m_uiCount;
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE bool plArrayBase<T, Derived>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::Clear()
{
  plMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, typename Derived>
bool plArrayBase<T, Derived>::Contains(const T& value) const
{
  return IndexOf(value) != plInvalidIndex;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::Insert(const T& value, plUInt32 uiIndex)
{
  PLASMA_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  plMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::Insert(T&& value, plUInt32 uiIndex)
{
  PLASMA_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  plMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::InsertRange(const plArrayPtr<const T>& range, plUInt32 uiIndex)
{
  const plUInt32 uiRangeCount = range.GetCount();
  static_cast<Derived*>(this)->Reserve(m_uiCount + uiRangeCount);

  plMemoryUtils::Prepend(static_cast<Derived*>(this)->GetElementsPtr() + uiIndex, range.GetPtr(), uiRangeCount, m_uiCount - uiIndex);
  m_uiCount += uiRangeCount;
}

template <typename T, typename Derived>
bool plArrayBase<T, Derived>::RemoveAndCopy(const T& value)
{
  plUInt32 uiIndex = IndexOf(value);

  if (uiIndex == plInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, typename Derived>
bool plArrayBase<T, Derived>::RemoveAndSwap(const T& value)
{
  plUInt32 uiIndex = IndexOf(value);

  if (uiIndex == plInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::RemoveAtAndCopy(plUInt32 uiIndex, plUInt32 uiNumElements /*= 1*/)
{
  PLASMA_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  m_uiCount -= uiNumElements;
  plMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::RemoveAtAndSwap(plUInt32 uiIndex, plUInt32 uiNumElements /*= 1*/)
{
  PLASMA_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  for (plUInt32 i = 0; i < uiNumElements; ++i)
  {
    m_uiCount--;

    if (m_uiCount != uiIndex)
    {
      pElements[uiIndex] = std::move(pElements[m_uiCount]);
    }
    plMemoryUtils::Destruct(pElements + m_uiCount, 1);
    ++uiIndex;
  }
}

template <typename T, typename Derived>
plUInt32 plArrayBase<T, Derived>::IndexOf(const T& value, plUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (plUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (plMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return plInvalidIndex;
}

template <typename T, typename Derived>
plUInt32 plArrayBase<T, Derived>::LastIndexOf(const T& value, plUInt32 uiStartIndex) const
{
  const T* pElements = static_cast<const Derived*>(this)->GetElementsPtr();

  for (plUInt32 i = plMath::Min(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (plMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return plInvalidIndex;
}

template <typename T, typename Derived>
T& plArrayBase<T, Derived>::ExpandAndGetRef()
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  T* pElements = static_cast<Derived*>(this)->GetElementsPtr();

  plMemoryUtils::Construct(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, typename Derived>
T* plArrayBase<T, Derived>::ExpandBy(plUInt32 uiNumNewItems)
{
  this->SetCount(this->GetCount() + uiNumNewItems);
  return GetArrayPtr().GetEndPtr() - uiNumNewItems;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::PushBack(const T& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  plMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::PushBack(T&& value)
{
  static_cast<Derived*>(this)->Reserve(m_uiCount + 1);

  plMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::PushBackUnchecked(const T& value)
{
  PLASMA_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  plMemoryUtils::CopyConstruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::PushBackUnchecked(T&& value)
{
  PLASMA_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  plMemoryUtils::MoveConstruct<T>(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::PushBackRange(const plArrayPtr<const T>& range)
{
  const plUInt32 uiRangeCount = range.GetCount();
  static_cast<Derived*>(this)->Reserve(m_uiCount + uiRangeCount);

  plMemoryUtils::CopyConstructArray(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::PopBack(plUInt32 uiCountToRemove /* = 1 */)
{
  PLASMA_ASSERT_DEV(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= uiCountToRemove;
  plMemoryUtils::Destruct(static_cast<Derived*>(this)->GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, typename Derived>
PLASMA_FORCE_INLINE T& plArrayBase<T, Derived>::PeekBack()
{
  PLASMA_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return static_cast<Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
PLASMA_FORCE_INLINE const T& plArrayBase<T, Derived>::PeekBack() const
{
  PLASMA_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return static_cast<const Derived*>(this)->GetElementsPtr()[m_uiCount - 1];
}

template <typename T, typename Derived>
template <typename Comparer>
void plArrayBase<T, Derived>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    plArrayPtr<T> ar = *this;
    plSorting::QuickSort(ar, comparer);
  }
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::Sort()
{
  if (m_uiCount > 1)
  {
    plArrayPtr<T> ar = *this;
    plSorting::QuickSort(ar, plCompareHelper<T>());
  }
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE T* plArrayBase<T, Derived>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return static_cast<Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE const T* plArrayBase<T, Derived>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return static_cast<const Derived*>(this)->GetElementsPtr();
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE plArrayPtr<T> plArrayBase<T, Derived>::GetArrayPtr()
{
  return plArrayPtr<T>(GetData(), GetCount());
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE plArrayPtr<const T> plArrayBase<T, Derived>::GetArrayPtr() const
{
  return plArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE plArrayPtr<typename plArrayPtr<T>::ByteType> plArrayBase<T, Derived>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
PLASMA_ALWAYS_INLINE plArrayPtr<typename plArrayPtr<const T>::ByteType> plArrayBase<T, Derived>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, typename Derived>
void plArrayBase<T, Derived>::DoSwap(plArrayBase<T, Derived>& other)
{
  plMath::Swap(this->m_pElements, other.m_pElements);
  plMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  plMath::Swap(this->m_uiCount, other.m_uiCount);
}
