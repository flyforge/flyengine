#pragma once

template <typename KEY, typename VALUE>
inline plArrayMapBase<KEY, VALUE>::plArrayMapBase(plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_bSorted = true;
}

template <typename KEY, typename VALUE>
inline plArrayMapBase<KEY, VALUE>::plArrayMapBase(const plArrayMapBase& rhs, plAllocatorBase* pAllocator)
  : m_bSorted(rhs.m_bSorted)
  , m_Data(pAllocator)
{
  m_Data = rhs.m_Data;
}

template <typename KEY, typename VALUE>
inline void plArrayMapBase<KEY, VALUE>::operator=(const plArrayMapBase& rhs)
{
  m_bSorted = rhs.m_bSorted;
  m_Data = rhs.m_Data;
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE plUInt32 plArrayMapBase<KEY, VALUE>::GetCount() const
{
  return m_Data.GetCount();
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE bool plArrayMapBase<KEY, VALUE>::IsEmpty() const
{
  return m_Data.IsEmpty();
}

template <typename KEY, typename VALUE>
inline void plArrayMapBase<KEY, VALUE>::Clear()
{
  m_bSorted = true;
  m_Data.Clear();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType, typename CompatibleValueType>
inline plUInt32 plArrayMapBase<KEY, VALUE>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value)
{
  Pair& ref = m_Data.ExpandAndGetRef();
  ref.key = std::forward<CompatibleKeyType>(key);
  ref.value = std::forward<CompatibleValueType>(value);
  m_bSorted = false;
  return m_Data.GetCount() - 1;
}

template <typename KEY, typename VALUE>
inline void plArrayMapBase<KEY, VALUE>::Sort() const
{
  if (m_bSorted)
    return;

  m_bSorted = true;
  m_Data.Sort();
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
plUInt32 plArrayMapBase<KEY, VALUE>::Find(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  plUInt32 lb = 0;
  plUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const plUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else // equal
    {
      return middle;
    }
  }

  return plInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
plUInt32 plArrayMapBase<KEY, VALUE>::LowerBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  plUInt32 lb = 0;
  plUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const plUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_Data[middle].key < key)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  if (lb == m_Data.GetCount())
    return plInvalidIndex;

  return lb;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
plUInt32 plArrayMapBase<KEY, VALUE>::UpperBound(const CompatibleKeyType& key) const
{
  if (!m_bSorted)
  {
    m_Data.Sort();
  }

  plUInt32 lb = 0;
  plUInt32 ub = m_Data.GetCount();

  while (lb < ub)
  {
    const plUInt32 middle = lb + ((ub - lb) >> 1);

    if (key < m_Data[middle].key)
    {
      ub = middle;
    }
    else
    {
      lb = middle + 1;
    }
  }

  if (ub == m_Data.GetCount())
    return plInvalidIndex;

  return ub;
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE const KEY& plArrayMapBase<KEY, VALUE>::GetKey(plUInt32 uiIndex) const
{
  return m_Data[uiIndex].key;
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE const VALUE& plArrayMapBase<KEY, VALUE>::GetValue(plUInt32 uiIndex) const
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
VALUE& plArrayMapBase<KEY, VALUE>::GetValue(plUInt32 uiIndex)
{
  return m_Data[uiIndex].value;
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE plDynamicArray<typename plArrayMapBase<KEY, VALUE>::Pair>& plArrayMapBase<KEY, VALUE>::GetData()
{
  m_bSorted = false;
  return m_Data;
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE const plDynamicArray<typename plArrayMapBase<KEY, VALUE>::Pair>& plArrayMapBase<KEY, VALUE>::GetData() const
{
  return m_Data;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
VALUE& plArrayMapBase<KEY, VALUE>::FindOrAdd(const CompatibleKeyType& key, bool* out_pExisted)
{
  plUInt32 index = Find<CompatibleKeyType>(key);

  if (out_pExisted)
    *out_pExisted = index != plInvalidIndex;

  if (index == plInvalidIndex)
  {
    index = Insert(key, VALUE());
  }

  return GetValue(index);
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
PLASMA_ALWAYS_INLINE VALUE& plArrayMapBase<KEY, VALUE>::operator[](const CompatibleKeyType& key)
{
  return FindOrAdd(key);
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE const typename plArrayMapBase<KEY, VALUE>::Pair& plArrayMapBase<KEY, VALUE>::GetPair(plUInt32 uiIndex) const
{
  return m_Data[uiIndex];
}

template <typename KEY, typename VALUE>
void plArrayMapBase<KEY, VALUE>::RemoveAtAndCopy(plUInt32 uiIndex, bool bKeepSorted)
{
  if (bKeepSorted && m_bSorted)
  {
    m_Data.RemoveAtAndCopy(uiIndex);
  }
  else
  {
    m_Data.RemoveAtAndSwap(uiIndex);
    m_bSorted = false;
  }
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool plArrayMapBase<KEY, VALUE>::RemoveAndCopy(const CompatibleKeyType& key, bool bKeepSorted)
{
  const plUInt32 uiIndex = Find(key);

  if (uiIndex == plInvalidIndex)
    return false;

  RemoveAtAndCopy(uiIndex, bKeepSorted);
  return true;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
PLASMA_ALWAYS_INLINE bool plArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key) const
{
  return Find(key) != plInvalidIndex;
}

template <typename KEY, typename VALUE>
template <typename CompatibleKeyType>
bool plArrayMapBase<KEY, VALUE>::Contains(const CompatibleKeyType& key, const VALUE& value) const
{
  plUInt32 atpos = LowerBound(key);

  if (atpos == plInvalidIndex)
    return false;

  while (atpos < m_Data.GetCount())
  {
    if (m_Data[atpos].key != key)
      return false;

    if (m_Data[atpos].value == value)
      return true;

    ++atpos;
  }

  return false;
}


template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE void plArrayMapBase<KEY, VALUE>::Reserve(plUInt32 uiSize)
{
  m_Data.Reserve(uiSize);
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE void plArrayMapBase<KEY, VALUE>::Compact()
{
  m_Data.Compact();
}

template <typename KEY, typename VALUE>
bool plArrayMapBase<KEY, VALUE>::operator==(const plArrayMapBase<KEY, VALUE>& rhs) const
{
  Sort();
  rhs.Sort();

  return m_Data == rhs.m_Data;
}

template <typename KEY, typename VALUE>
PLASMA_ALWAYS_INLINE bool plArrayMapBase<KEY, VALUE>::operator!=(const plArrayMapBase<KEY, VALUE>& rhs) const
{
  return !(*this == rhs);
}

template <typename KEY, typename VALUE, typename A>
plArrayMap<KEY, VALUE, A>::plArrayMap()
  : plArrayMapBase<KEY, VALUE>(A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
plArrayMap<KEY, VALUE, A>::plArrayMap(plAllocatorBase* pAllocator)
  : plArrayMapBase<KEY, VALUE>(pAllocator)
{
}

template <typename KEY, typename VALUE, typename A>
plArrayMap<KEY, VALUE, A>::plArrayMap(const plArrayMap<KEY, VALUE, A>& rhs)
  : plArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
plArrayMap<KEY, VALUE, A>::plArrayMap(const plArrayMapBase<KEY, VALUE>& rhs)
  : plArrayMapBase<KEY, VALUE>(rhs, A::GetAllocator())
{
}

template <typename KEY, typename VALUE, typename A>
void plArrayMap<KEY, VALUE, A>::operator=(const plArrayMap<KEY, VALUE, A>& rhs)
{
  plArrayMapBase<KEY, VALUE>::operator=(rhs);
}

template <typename KEY, typename VALUE, typename A>
void plArrayMap<KEY, VALUE, A>::operator=(const plArrayMapBase<KEY, VALUE>& rhs)
{
  plArrayMapBase<KEY, VALUE>::operator=(rhs);
}
