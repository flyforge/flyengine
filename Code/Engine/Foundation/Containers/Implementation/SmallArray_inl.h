
template <typename T, plUInt16 Size>
plSmallArrayBase<T, Size>::plSmallArrayBase() = default;

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plSmallArrayBase<T, Size>::plSmallArrayBase(const plSmallArrayBase<T, Size>& other, plAllocator* pAllocator)
{
  CopyFrom((plArrayPtr<const T>)other, pAllocator);
  m_uiUserData = other.m_uiUserData;
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plSmallArrayBase<T, Size>::plSmallArrayBase(const plArrayPtr<const T>& other, plAllocator* pAllocator)
{
  CopyFrom(other, pAllocator);
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plSmallArrayBase<T, Size>::plSmallArrayBase(plSmallArrayBase<T, Size>&& other, plAllocator* pAllocator)
{
  MoveFrom(std::move(other), pAllocator);
}

template <typename T, plUInt16 Size>
PL_FORCE_INLINE plSmallArrayBase<T, Size>::~plSmallArrayBase()
{
  PL_ASSERT_DEBUG(m_uiCount == 0, "The derived class did not destruct all objects. Count is {0}.", m_uiCount);
  PL_ASSERT_DEBUG(m_pElements == nullptr, "The derived class did not free its memory.");
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::CopyFrom(const plArrayPtr<const T>& other, plAllocator* pAllocator)
{
  PL_ASSERT_DEV(other.GetCount() <= plSmallInvalidIndex, "Can't copy {} elements to small array. Maximum count is {}", other.GetCount(), plSmallInvalidIndex);

  if (GetData() == other.GetPtr())
  {
    if (m_uiCount == other.GetCount())
      return;

    PL_ASSERT_DEV(m_uiCount > other.GetCount(), "Dangling array pointer. The given array pointer points to invalid memory.");
    T* pElements = GetElementsPtr();
    plMemoryUtils::Destruct(pElements + other.GetCount(), m_uiCount - other.GetCount());
    m_uiCount = static_cast<plUInt16>(other.GetCount());
    return;
  }

  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = other.GetCount();

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<plUInt16>(uiNewCount), pAllocator);
    T* pElements = GetElementsPtr();
    plMemoryUtils::Copy(pElements, other.GetPtr(), uiOldCount);
    plMemoryUtils::CopyConstructArray(pElements + uiOldCount, other.GetPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else
  {
    T* pElements = GetElementsPtr();
    plMemoryUtils::Copy(pElements, other.GetPtr(), uiNewCount);
    plMemoryUtils::Destruct(pElements + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = static_cast<plUInt16>(uiNewCount);
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::MoveFrom(plSmallArrayBase<T, Size>&& other, plAllocator* pAllocator)
{
  Clear();

  if (other.m_uiCapacity > Size)
  {
    if (m_uiCapacity > Size)
    {
      // only delete our own external storage
      PL_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = other.m_uiCapacity;
    m_pElements = other.m_pElements;
  }
  else
  {
    plMemoryUtils::RelocateConstruct(GetElementsPtr(), other.GetElementsPtr(), other.m_uiCount);
  }

  m_uiCount = other.m_uiCount;
  m_uiUserData = other.m_uiUserData;

  // reset the other array to not reference the data anymore
  other.m_pElements = nullptr;
  other.m_uiCount = 0;
  other.m_uiCapacity = 0;
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plSmallArrayBase<T, Size>::operator plArrayPtr<const T>() const
{
  return plArrayPtr<const T>(GetElementsPtr(), m_uiCount);
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plSmallArrayBase<T, Size>::operator plArrayPtr<T>()
{
  return plArrayPtr<T>(GetElementsPtr(), m_uiCount);
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE bool plSmallArrayBase<T, Size>::operator==(const plSmallArrayBase<T, Size>& rhs) const
{
  return *this == rhs.GetArrayPtr();
}

#if PL_DISABLED(PL_USE_CPP20_OPERATORS)
template <typename T, plUInt16 Size>
bool plSmallArrayBase<T, Size>::operator==(const plArrayPtr<const T>& rhs) const
{
  if (m_uiCount != rhs.GetCount())
    return false;

  return plMemoryUtils::IsEqual(GetElementsPtr(), rhs.GetPtr(), m_uiCount);
}
#endif

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE const T& plSmallArrayBase<T, Size>::operator[](const plUInt32 uiIndex) const
{
  PL_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE T& plSmallArrayBase<T, Size>::operator[](const plUInt32 uiIndex)
{
  PL_ASSERT_DEBUG(uiIndex < m_uiCount, "Out of bounds access. Array has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return GetElementsPtr()[uiIndex];
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::SetCount(plUInt16 uiCount, plAllocator* pAllocator)
{
  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(static_cast<plUInt16>(uiNewCount), pAllocator);
    plMemoryUtils::Construct<ConstructAll>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    plMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::SetCount(plUInt16 uiCount, const T& fillValue, plAllocator* pAllocator)
{
  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiCount, pAllocator);
    plMemoryUtils::CopyConstruct(GetElementsPtr() + uiOldCount, fillValue, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    plMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::EnsureCount(plUInt16 uiCount, plAllocator* pAllocator)
{
  if (uiCount > m_uiCount)
  {
    SetCount(uiCount, pAllocator);
  }
}

template <typename T, plUInt16 Size>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
void plSmallArrayBase<T, Size>::SetCountUninitialized(plUInt16 uiCount, plAllocator* pAllocator)
{
  static_assert(plIsPodType<T>::value == plTypeIsPod::value, "SetCountUninitialized is only supported for POD types.");
  const plUInt32 uiOldCount = m_uiCount;
  const plUInt32 uiNewCount = uiCount;

  if (uiNewCount > uiOldCount)
  {
    Reserve(uiNewCount, pAllocator);
    plMemoryUtils::Construct<SkipTrivialTypes>(GetElementsPtr() + uiOldCount, uiNewCount - uiOldCount);
  }
  else if (uiNewCount < uiOldCount)
  {
    plMemoryUtils::Destruct(GetElementsPtr() + uiNewCount, uiOldCount - uiNewCount);
  }

  m_uiCount = uiCount;
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plUInt32 plSmallArrayBase<T, Size>::GetCount() const
{
  return m_uiCount;
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE bool plSmallArrayBase<T, Size>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::Clear()
{
  plMemoryUtils::Destruct(GetElementsPtr(), m_uiCount);
  m_uiCount = 0;
}

template <typename T, plUInt16 Size>
bool plSmallArrayBase<T, Size>::Contains(const T& value) const
{
  return IndexOf(value) != plInvalidIndex;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::Insert(const T& value, plUInt32 uiIndex, plAllocator* pAllocator)
{
  PL_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  plMemoryUtils::Prepend(GetElementsPtr() + uiIndex, value, m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::Insert(T&& value, plUInt32 uiIndex, plAllocator* pAllocator)
{
  PL_ASSERT_DEV(uiIndex <= m_uiCount, "Invalid index. Array has {0} elements, trying to insert element at index {1}.", m_uiCount, uiIndex);

  Reserve(m_uiCount + 1, pAllocator);

  plMemoryUtils::Prepend(GetElementsPtr() + uiIndex, std::move(value), m_uiCount - uiIndex);
  m_uiCount++;
}

template <typename T, plUInt16 Size>
bool plSmallArrayBase<T, Size>::RemoveAndCopy(const T& value)
{
  plUInt32 uiIndex = IndexOf(value);

  if (uiIndex == plInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex);
  return true;
}

template <typename T, plUInt16 Size>
bool plSmallArrayBase<T, Size>::RemoveAndSwap(const T& value)
{
  plUInt32 uiIndex = IndexOf(value);

  if (uiIndex == plInvalidIndex)
    return false;

  RemoveAtAndSwap(uiIndex);
  return true;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::RemoveAtAndCopy(plUInt32 uiIndex, plUInt16 uiNumElements /*= 1*/)
{
  PL_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

  m_uiCount -= uiNumElements;
  plMemoryUtils::RelocateOverlapped(pElements + uiIndex, pElements + uiIndex + uiNumElements, m_uiCount - uiIndex);
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::RemoveAtAndSwap(plUInt32 uiIndex, plUInt16 uiNumElements /*= 1*/)
{
  PL_ASSERT_DEV(uiIndex + uiNumElements <= m_uiCount, "Out of bounds access. Array has {0} elements, trying to remove element at index {1}.", m_uiCount, uiIndex + uiNumElements - 1);

  T* pElements = GetElementsPtr();

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

template <typename T, plUInt16 Size>
plUInt32 plSmallArrayBase<T, Size>::IndexOf(const T& value, plUInt32 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (plUInt32 i = uiStartIndex; i < m_uiCount; i++)
  {
    if (plMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return plInvalidIndex;
}

template <typename T, plUInt16 Size>
plUInt32 plSmallArrayBase<T, Size>::LastIndexOf(const T& value, plUInt32 uiStartIndex) const
{
  const T* pElements = GetElementsPtr();

  for (plUInt32 i = plMath::Min<plUInt32>(uiStartIndex, m_uiCount); i-- > 0;)
  {
    if (plMemoryUtils::IsEqual(pElements + i, &value))
      return i;
  }
  return plInvalidIndex;
}

template <typename T, plUInt16 Size>
T& plSmallArrayBase<T, Size>::ExpandAndGetRef(plAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  T* pElements = GetElementsPtr();

  plMemoryUtils::Construct<SkipTrivialTypes>(pElements + m_uiCount, 1);

  T& ReturnRef = *(pElements + m_uiCount);

  m_uiCount++;

  return ReturnRef;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::PushBack(const T& value, plAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  plMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::PushBack(T&& value, plAllocator* pAllocator)
{
  Reserve(m_uiCount + 1, pAllocator);

  plMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::PushBackUnchecked(const T& value)
{
  PL_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  plMemoryUtils::CopyConstruct(GetElementsPtr() + m_uiCount, value, 1);
  m_uiCount++;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::PushBackUnchecked(T&& value)
{
  PL_ASSERT_DEBUG(m_uiCount < m_uiCapacity, "Appending unchecked to array with insufficient capacity.");

  plMemoryUtils::MoveConstruct<T>(GetElementsPtr() + m_uiCount, std::move(value));
  m_uiCount++;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::PushBackRange(const plArrayPtr<const T>& range, plAllocator* pAllocator)
{
  const plUInt32 uiRangeCount = range.GetCount();
  Reserve(m_uiCount + uiRangeCount, pAllocator);

  plMemoryUtils::CopyConstructArray(GetElementsPtr() + m_uiCount, range.GetPtr(), uiRangeCount);
  m_uiCount += uiRangeCount;
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::PopBack(plUInt32 uiCountToRemove /* = 1 */)
{
  PL_ASSERT_DEBUG(m_uiCount >= uiCountToRemove, "Out of bounds access. Array has {0} elements, trying to pop {1} elements.", m_uiCount, uiCountToRemove);

  m_uiCount -= uiCountToRemove;
  plMemoryUtils::Destruct(GetElementsPtr() + m_uiCount, uiCountToRemove);
}

template <typename T, plUInt16 Size>
PL_FORCE_INLINE T& plSmallArrayBase<T, Size>::PeekBack()
{
  PL_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, plUInt16 Size>
PL_FORCE_INLINE const T& plSmallArrayBase<T, Size>::PeekBack() const
{
  PL_ASSERT_DEBUG(m_uiCount > 0, "Out of bounds access. Trying to peek into an empty array.");
  return GetElementsPtr()[m_uiCount - 1];
}

template <typename T, plUInt16 Size>
template <typename Comparer>
void plSmallArrayBase<T, Size>::Sort(const Comparer& comparer)
{
  if (m_uiCount > 1)
  {
    plArrayPtr<T> ar = GetArrayPtr();
    plSorting::QuickSort(ar, comparer);
  }
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::Sort()
{
  if (m_uiCount > 1)
  {
    plArrayPtr<T> ar = GetArrayPtr();
    plSorting::QuickSort(ar, plCompareHelper<T>());
  }
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE T* plSmallArrayBase<T, Size>::GetData()
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE const T* plSmallArrayBase<T, Size>::GetData() const
{
  if (IsEmpty())
    return nullptr;

  return GetElementsPtr();
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plArrayPtr<T> plSmallArrayBase<T, Size>::GetArrayPtr()
{
  return plArrayPtr<T>(GetData(), GetCount());
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plArrayPtr<const T> plSmallArrayBase<T, Size>::GetArrayPtr() const
{
  return plArrayPtr<const T>(GetData(), GetCount());
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plArrayPtr<typename plArrayPtr<T>::ByteType> plSmallArrayBase<T, Size>::GetByteArrayPtr()
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plArrayPtr<typename plArrayPtr<const T>::ByteType> plSmallArrayBase<T, Size>::GetByteArrayPtr() const
{
  return GetArrayPtr().ToByteArray();
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::Reserve(plUInt16 uiCapacity, plAllocator* pAllocator)
{
  if (m_uiCapacity >= uiCapacity)
    return;

  const plUInt32 uiCurCap = static_cast<plUInt32>(m_uiCapacity);
  plUInt32 uiNewCapacity = uiCurCap + (uiCurCap / 2);

  uiNewCapacity = plMath::Max<plUInt32>(uiNewCapacity, uiCapacity);
  uiNewCapacity = plMemoryUtils::AlignSize<plUInt32>(uiNewCapacity, CAPACITY_ALIGNMENT);
  uiNewCapacity = plMath::Min<plUInt32>(uiNewCapacity, 0xFFFFu);

  SetCapacity(static_cast<plUInt16>(uiNewCapacity), pAllocator);
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::Compact(plAllocator* pAllocator)
{
  if (IsEmpty())
  {
    if (m_uiCapacity > Size)
    {
      // completely deallocate all data, if the array is empty.
      PL_DELETE_RAW_BUFFER(pAllocator, m_pElements);
    }

    m_uiCapacity = Size;
    m_pElements = nullptr;
  }
  else if (m_uiCapacity > Size)
  {
    plUInt32 uiNewCapacity = plMemoryUtils::AlignSize<plUInt32>(m_uiCount, CAPACITY_ALIGNMENT);
    uiNewCapacity = plMath::Min<plUInt32>(uiNewCapacity, 0xFFFFu);

    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(static_cast<plUInt16>(uiNewCapacity), pAllocator);
  }
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE plUInt64 plSmallArrayBase<T, Size>::GetHeapMemoryUsage() const
{
  return m_uiCapacity <= Size ? 0 : m_uiCapacity * sizeof(T);
}

template <typename T, plUInt16 Size>
template <typename U>
PL_ALWAYS_INLINE const U& plSmallArrayBase<T, Size>::GetUserData() const
{
  static_assert(sizeof(U) <= sizeof(plUInt32));
  return reinterpret_cast<const U&>(m_uiUserData);
}

template <typename T, plUInt16 Size>
template <typename U>
PL_ALWAYS_INLINE U& plSmallArrayBase<T, Size>::GetUserData()
{
  static_assert(sizeof(U) <= sizeof(plUInt32));
  return reinterpret_cast<U&>(m_uiUserData);
}

template <typename T, plUInt16 Size>
void plSmallArrayBase<T, Size>::SetCapacity(plUInt16 uiCapacity, plAllocator* pAllocator)
{
  if (m_uiCapacity > Size && uiCapacity > m_uiCapacity)
  {
    m_pElements = PL_EXTEND_RAW_BUFFER(pAllocator, m_pElements, m_uiCount, uiCapacity);
    m_uiCapacity = uiCapacity;
  }
  else
  {
    // special case when migrating from in-place to external storage or shrinking
    T* pOldElements = GetElementsPtr();

    const plUInt32 uiOldCapacity = m_uiCapacity;
    const plUInt32 uiNewCapacity = uiCapacity;
    m_uiCapacity = plMath::Max(uiCapacity, Size);

    if (uiNewCapacity > Size)
    {
      // new external storage
      T* pNewElements = PL_NEW_RAW_BUFFER(pAllocator, T, uiCapacity);
      plMemoryUtils::RelocateConstruct(pNewElements, pOldElements, m_uiCount);
      m_pElements = pNewElements;
    }
    else
    {
      // Re-use inplace storage
      plMemoryUtils::RelocateConstruct(GetElementsPtr(), pOldElements, m_uiCount);
    }

    if (uiOldCapacity > Size)
    {
      PL_DELETE_RAW_BUFFER(pAllocator, pOldElements);
    }
  }
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE T* plSmallArrayBase<T, Size>::GetElementsPtr()
{
  return m_uiCapacity <= Size ? reinterpret_cast<T*>(m_StaticData) : m_pElements;
}

template <typename T, plUInt16 Size>
PL_ALWAYS_INLINE const T* plSmallArrayBase<T, Size>::GetElementsPtr() const
{
  return m_uiCapacity <= Size ? reinterpret_cast<const T*>(m_StaticData) : m_pElements;
}

//////////////////////////////////////////////////////////////////////////

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
plSmallArray<T, Size, AllocatorWrapper>::plSmallArray() = default;

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE plSmallArray<T, Size, AllocatorWrapper>::plSmallArray(const plSmallArray<T, Size, AllocatorWrapper>& other)
  : SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE plSmallArray<T, Size, AllocatorWrapper>::plSmallArray(const plArrayPtr<const T>& other)
  : SUPER(other, AllocatorWrapper::GetAllocator())
{
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE plSmallArray<T, Size, AllocatorWrapper>::plSmallArray(plSmallArray<T, Size, AllocatorWrapper>&& other)
  : SUPER(static_cast<SUPER&&>(other), AllocatorWrapper::GetAllocator())
{
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
plSmallArray<T, Size, AllocatorWrapper>::~plSmallArray()
{
  SUPER::Clear();
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::operator=(const plSmallArray<T, Size, AllocatorWrapper>& rhs)
{
  *this = ((plArrayPtr<const T>)rhs); // redirect this to the plArrayPtr version
  this->m_uiUserData = rhs.m_uiUserData;
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::operator=(const plArrayPtr<const T>& rhs)
{
  SUPER::CopyFrom(rhs, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::operator=(plSmallArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  SUPER::MoveFrom(std::move(rhs), AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::SetCount(plUInt16 uiCount)
{
  SUPER::SetCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::SetCount(plUInt16 uiCount, const T& fillValue)
{
  SUPER::SetCount(uiCount, fillValue, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::EnsureCount(plUInt16 uiCount)
{
  SUPER::EnsureCount(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
template <typename> // Second template needed so that the compiler does only instantiate it when called. Otherwise the static_assert would trigger early.
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::SetCountUninitialized(plUInt16 uiCount)
{
  SUPER::SetCountUninitialized(uiCount, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::Insert(const T& value, plUInt32 uiIndex)
{
  SUPER::Insert(value, uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::Insert(T&& value, plUInt32 uiIndex)
{
  SUPER::Insert(value, uiIndex, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE T& plSmallArray<T, Size, AllocatorWrapper>::ExpandAndGetRef()
{
  return SUPER::ExpandAndGetRef(AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::PushBack(const T& value)
{
  SUPER::PushBack(value, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::PushBack(T&& value)
{
  SUPER::PushBack(std::move(value), AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::PushBackRange(const plArrayPtr<const T>& range)
{
  SUPER::PushBackRange(range, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::Reserve(plUInt16 uiCapacity)
{
  SUPER::Reserve(uiCapacity, AllocatorWrapper::GetAllocator());
}

template <typename T, plUInt16 Size, typename AllocatorWrapper /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plSmallArray<T, Size, AllocatorWrapper>::Compact()
{
  SUPER::Compact(AllocatorWrapper::GetAllocator());
}

//////////////////////////////////////////////////////////////////////////

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::iterator begin(plSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData();
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_iterator begin(const plSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_iterator cbegin(const plSmallArrayBase<T, Size>& container)
{
  return container.GetData();
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::reverse_iterator rbegin(plSmallArrayBase<T, Size>& ref_container)
{
  return typename plSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() + ref_container.GetCount() - 1);
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_reverse_iterator rbegin(const plSmallArrayBase<T, Size>& container)
{
  return typename plSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_reverse_iterator crbegin(const plSmallArrayBase<T, Size>& container)
{
  return typename plSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() + container.GetCount() - 1);
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::iterator end(plSmallArrayBase<T, Size>& ref_container)
{
  return ref_container.GetData() + ref_container.GetCount();
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_iterator end(const plSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_iterator cend(const plSmallArrayBase<T, Size>& container)
{
  return container.GetData() + container.GetCount();
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::reverse_iterator rend(plSmallArrayBase<T, Size>& ref_container)
{
  return typename plSmallArrayBase<T, Size>::reverse_iterator(ref_container.GetData() - 1);
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_reverse_iterator rend(const plSmallArrayBase<T, Size>& container)
{
  return typename plSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}

template <typename T, plUInt16 Size>
typename plSmallArrayBase<T, Size>::const_reverse_iterator crend(const plSmallArrayBase<T, Size>& container)
{
  return typename plSmallArrayBase<T, Size>::const_reverse_iterator(container.GetData() - 1);
}
