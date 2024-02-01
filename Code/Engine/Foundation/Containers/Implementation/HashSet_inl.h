
/// \brief Value used by containers for indices to indicate an invalid index.
#ifndef plInvalidIndex
#  define plInvalidIndex 0xFFFFFFFF
#endif

// ***** Const Iterator *****

template <typename K, typename H>
plHashSetBase<K, H>::ConstIterator::ConstIterator(const plHashSetBase<K, H>& hashSet)
  : m_pHashSet(&hashSet)
{
}

template <typename K, typename H>
void plHashSetBase<K, H>::ConstIterator::SetToBegin()
{
  if (m_pHashSet->IsEmpty())
  {
    m_uiCurrentIndex = m_pHashSet->m_uiCapacity;
    return;
  }
  while (!m_pHashSet->IsValidEntry(m_uiCurrentIndex))
  {
    ++m_uiCurrentIndex;
  }
}

template <typename K, typename H>
inline void plHashSetBase<K, H>::ConstIterator::SetToEnd()
{
  m_uiCurrentCount = m_pHashSet->m_uiCount;
  m_uiCurrentIndex = m_pHashSet->m_uiCapacity;
}

template <typename K, typename H>
PL_ALWAYS_INLINE bool plHashSetBase<K, H>::ConstIterator::IsValid() const
{
  return m_uiCurrentCount < m_pHashSet->m_uiCount;
}

template <typename K, typename H>
PL_ALWAYS_INLINE bool plHashSetBase<K, H>::ConstIterator::operator==(const typename plHashSetBase<K, H>::ConstIterator& rhs) const
{
  return m_uiCurrentIndex == rhs.m_uiCurrentIndex && m_pHashSet->m_pEntries == rhs.m_pHashSet->m_pEntries;
}

template <typename K, typename H>
PL_FORCE_INLINE const K& plHashSetBase<K, H>::ConstIterator::Key() const
{
  return m_pHashSet->m_pEntries[m_uiCurrentIndex];
}

template <typename K, typename H>
void plHashSetBase<K, H>::ConstIterator::Next()
{
  ++m_uiCurrentCount;
  if (m_uiCurrentCount == m_pHashSet->m_uiCount)
  {
    m_uiCurrentIndex = m_pHashSet->m_uiCapacity;
    return;
  }

  do
  {
    ++m_uiCurrentIndex;
  } while (!m_pHashSet->IsValidEntry(m_uiCurrentIndex));
}

template <typename K, typename H>
PL_ALWAYS_INLINE void plHashSetBase<K, H>::ConstIterator::operator++()
{
  Next();
}


// ***** plHashSetBase *****

template <typename K, typename H>
plHashSetBase<K, H>::plHashSetBase(plAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;
}

template <typename K, typename H>
plHashSetBase<K, H>::plHashSetBase(const plHashSetBase<K, H>& other, plAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename K, typename H>
plHashSetBase<K, H>::plHashSetBase(plHashSetBase<K, H>&& other, plAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename K, typename H>
plHashSetBase<K, H>::~plHashSetBase()
{
  Clear();
  PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
  m_uiCapacity = 0;
}

template <typename K, typename H>
void plHashSetBase<K, H>::operator=(const plHashSetBase<K, H>& rhs)
{
  Clear();
  Reserve(rhs.GetCount());

  plUInt32 uiCopied = 0;
  for (plUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
  {
    if (rhs.IsValidEntry(i))
    {
      Insert(rhs.m_pEntries[i]);
      ++uiCopied;
    }
  }
}

template <typename K, typename H>
void plHashSetBase<K, H>::operator=(plHashSetBase<K, H>&& rhs)
{
  // Clear any existing data (calls destructors if necessary)
  Clear();

  if (m_pAllocator != rhs.m_pAllocator)
  {
    Reserve(rhs.m_uiCapacity);

    plUInt32 uiCopied = 0;
    for (plUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
    {
      if (rhs.IsValidEntry(i))
      {
        Insert(std::move(rhs.m_pEntries[i]));
        ++uiCopied;
      }
    }

    rhs.Clear();
  }
  else
  {
    PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);

    // Move all data over.
    m_pEntries = rhs.m_pEntries;
    m_pEntryFlags = rhs.m_pEntryFlags;
    m_uiCount = rhs.m_uiCount;
    m_uiCapacity = rhs.m_uiCapacity;

    // Temp copy forgets all its state.
    rhs.m_pEntries = nullptr;
    rhs.m_pEntryFlags = nullptr;
    rhs.m_uiCount = 0;
    rhs.m_uiCapacity = 0;
  }
}

template <typename K, typename H>
bool plHashSetBase<K, H>::operator==(const plHashSetBase<K, H>& rhs) const
{
  if (m_uiCount != rhs.m_uiCount)
    return false;

  plUInt32 uiCompared = 0;
  for (plUInt32 i = 0; uiCompared < m_uiCount; ++i)
  {
    if (IsValidEntry(i))
    {
      if (!rhs.Contains(m_pEntries[i]))
        return false;

      ++uiCompared;
    }
  }

  return true;
}

template <typename K, typename H>
void plHashSetBase<K, H>::Reserve(plUInt32 uiCapacity)
{
  const plUInt64 uiCap64 = static_cast<plUInt64>(uiCapacity);
  plUInt64 uiNewCapacity64 = uiCap64 + (uiCap64 * 2 / 3); // ensure a maximum load of 60%

  uiNewCapacity64 = plMath::Min<plUInt64>(uiNewCapacity64, 0x80000000llu); // the largest power-of-two in 32 bit

  plUInt32 uiNewCapacity32 = static_cast<plUInt32>(uiNewCapacity64 & 0xFFFFFFFF);
  PL_ASSERT_DEBUG(uiCapacity <= uiNewCapacity32, "plHashSet/Map do not support more than 2 billion entries.");

  if (m_uiCapacity >= uiNewCapacity32)
    return;

  uiNewCapacity32 = plMath::Max<plUInt32>(plMath::PowerOfTwo_Ceil(uiNewCapacity32), CAPACITY_ALIGNMENT);
  SetCapacity(uiNewCapacity32);
}

template <typename K, typename H>
void plHashSetBase<K, H>::Compact()
{
  if (IsEmpty())
  {
    // completely deallocate all data, if the table is empty.
    PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
    m_uiCapacity = 0;
  }
  else
  {
    const plUInt32 uiNewCapacity = (m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename K, typename H>
PL_ALWAYS_INLINE plUInt32 plHashSetBase<K, H>::GetCount() const
{
  return m_uiCount;
}

template <typename K, typename H>
PL_ALWAYS_INLINE bool plHashSetBase<K, H>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename K, typename H>
void plHashSetBase<K, H>::Clear()
{
  for (plUInt32 i = 0; i < m_uiCapacity; ++i)
  {
    if (IsValidEntry(i))
    {
      plMemoryUtils::Destruct(&m_pEntries[i], 1);
    }
  }

  plMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());
  m_uiCount = 0;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
bool plHashSetBase<K, H>::Insert(CompatibleKeyType&& key)
{
  Reserve(m_uiCount + 1);

  plUInt32 uiIndex = H::Hash(key) & (m_uiCapacity - 1);
  plUInt32 uiDeletedIndex = plInvalidIndex;

  plUInt32 uiCounter = 0;
  while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
  {
    if (IsDeletedEntry(uiIndex))
    {
      if (uiDeletedIndex == plInvalidIndex)
        uiDeletedIndex = uiIndex;
    }
    else if (H::Equal(m_pEntries[uiIndex], key))
    {
      return true;
    }
    ++uiIndex;
    if (uiIndex == m_uiCapacity)
      uiIndex = 0;

    ++uiCounter;
  }

  // new entry
  uiIndex = uiDeletedIndex != plInvalidIndex ? uiDeletedIndex : uiIndex;

  // Constructions might either be a move or a copy.
  plMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex], std::forward<CompatibleKeyType>(key));

  MarkEntryAsValid(uiIndex);
  ++m_uiCount;

  return false;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
bool plHashSetBase<K, H>::Remove(const CompatibleKeyType& key)
{
  plUInt32 uiIndex = FindEntry(key);
  if (uiIndex != plInvalidIndex)
  {
    RemoveInternal(uiIndex);
    return true;
  }

  return false;
}

template <typename K, typename H>
typename plHashSetBase<K, H>::ConstIterator plHashSetBase<K, H>::Remove(const typename plHashSetBase<K, H>::ConstIterator& pos)
{
  ConstIterator it = pos;
  plUInt32 uiIndex = pos.m_uiCurrentIndex;
  ++it;
  --it.m_uiCurrentCount;
  RemoveInternal(uiIndex);
  return it;
}

template <typename K, typename H>
void plHashSetBase<K, H>::RemoveInternal(plUInt32 uiIndex)
{
  plMemoryUtils::Destruct(&m_pEntries[uiIndex], 1);

  plUInt32 uiNextIndex = uiIndex + 1;
  if (uiNextIndex == m_uiCapacity)
    uiNextIndex = 0;

  // if the next entry is free we are at the end of a chain and
  // can immediately mark this entry as free as well
  if (IsFreeEntry(uiNextIndex))
  {
    MarkEntryAsFree(uiIndex);

    // run backwards and free all deleted entries in this chain
    plUInt32 uiPrevIndex = (uiIndex != 0) ? uiIndex : m_uiCapacity;
    --uiPrevIndex;

    while (IsDeletedEntry(uiPrevIndex))
    {
      MarkEntryAsFree(uiPrevIndex);

      if (uiPrevIndex == 0)
        uiPrevIndex = m_uiCapacity;
      --uiPrevIndex;
    }
  }
  else
  {
    MarkEntryAsDeleted(uiIndex);
  }

  --m_uiCount;
}

template <typename K, typename H>
template <typename CompatibleKeyType>
PL_FORCE_INLINE bool plHashSetBase<K, H>::Contains(const CompatibleKeyType& key) const
{
  return FindEntry(key) != plInvalidIndex;
}

template <typename K, typename H>
bool plHashSetBase<K, H>::ContainsSet(const plHashSetBase<K, H>& operand) const
{
  for (const K& key : operand)
  {
    if (!Contains(key))
      return false;
  }

  return true;
}

template <typename K, typename H>
void plHashSetBase<K, H>::Union(const plHashSetBase<K, H>& operand)
{
  Reserve(GetCount() + operand.GetCount());
  for (const auto& key : operand)
  {
    Insert(key);
  }
}

template <typename K, typename H>
void plHashSetBase<K, H>::Difference(const plHashSetBase<K, H>& operand)
{
  for (const auto& key : operand)
  {
    Remove(key);
  }
}

template <typename K, typename H>
void plHashSetBase<K, H>::Intersection(const plHashSetBase<K, H>& operand)
{
  for (auto it = GetIterator(); it.IsValid();)
  {
    if (!operand.Contains(it.Key()))
      it = Remove(it);
    else
      ++it;
  }
}

template <typename K, typename H>
PL_FORCE_INLINE typename plHashSetBase<K, H>::ConstIterator plHashSetBase<K, H>::GetIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename H>
PL_FORCE_INLINE typename plHashSetBase<K, H>::ConstIterator plHashSetBase<K, H>::GetEndIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename H>
PL_ALWAYS_INLINE plAllocator* plHashSetBase<K, H>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename K, typename H>
plUInt64 plHashSetBase<K, H>::GetHeapMemoryUsage() const
{
  return ((plUInt64)m_uiCapacity * sizeof(K)) + (sizeof(plUInt32) * (plUInt64)GetFlagsCapacity());
}

// private methods
template <typename K, typename H>
void plHashSetBase<K, H>::SetCapacity(plUInt32 uiCapacity)
{
  PL_ASSERT_DEBUG(plMath::IsPowerOf2(uiCapacity), "uiCapacity must be a power of two to avoid modulo during lookup.");
  const plUInt32 uiOldCapacity = m_uiCapacity;
  m_uiCapacity = uiCapacity;

  K* pOldEntries = m_pEntries;
  plUInt32* pOldEntryFlags = m_pEntryFlags;

  m_pEntries = PL_NEW_RAW_BUFFER(m_pAllocator, K, m_uiCapacity);
  m_pEntryFlags = PL_NEW_RAW_BUFFER(m_pAllocator, plUInt32, GetFlagsCapacity());
  plMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());

  m_uiCount = 0;
  for (plUInt32 i = 0; i < uiOldCapacity; ++i)
  {
    if (GetFlags(pOldEntryFlags, i) == VALID_ENTRY)
    {
      PL_VERIFY(!Insert(std::move(pOldEntries[i])), "Implementation error");

      plMemoryUtils::Destruct(&pOldEntries[i], 1);
    }
  }

  PL_DELETE_RAW_BUFFER(m_pAllocator, pOldEntries);
  PL_DELETE_RAW_BUFFER(m_pAllocator, pOldEntryFlags);
}

template <typename K, typename H>
template <typename CompatibleKeyType>
PL_FORCE_INLINE plUInt32 plHashSetBase<K, H>::FindEntry(const CompatibleKeyType& key) const
{
  return FindEntry(H::Hash(key), key);
}

template <typename K, typename H>
template <typename CompatibleKeyType>
inline plUInt32 plHashSetBase<K, H>::FindEntry(plUInt32 uiHash, const CompatibleKeyType& key) const
{
  if (m_uiCapacity > 0)
  {
    plUInt32 uiIndex = uiHash & (m_uiCapacity - 1);
    plUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsValidEntry(uiIndex) && H::Equal(m_pEntries[uiIndex], key))
        return uiIndex;

      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;

      ++uiCounter;
    }
  }
  // not found
  return plInvalidIndex;
}

#define PL_HASHSET_USE_BITFLAGS PL_ON

template <typename K, typename H>
PL_FORCE_INLINE plUInt32 plHashSetBase<K, H>::GetFlagsCapacity() const
{
#if PL_ENABLED(PL_HASHSET_USE_BITFLAGS)
  return (m_uiCapacity + 15) / 16;
#else
  return m_uiCapacity;
#endif
}

template <typename K, typename H>
plUInt32 plHashSetBase<K, H>::GetFlags(plUInt32* pFlags, plUInt32 uiEntryIndex) const
{
#if PL_ENABLED(PL_HASHSET_USE_BITFLAGS)
  const plUInt32 uiIndex = uiEntryIndex / 16;
  const plUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
#else
  return pFlags[uiEntryIndex] & FLAGS_MASK;
#endif
}

template <typename K, typename H>
void plHashSetBase<K, H>::SetFlags(plUInt32 uiEntryIndex, plUInt32 uiFlags)
{
#if PL_ENABLED(PL_HASHSET_USE_BITFLAGS)
  const plUInt32 uiIndex = uiEntryIndex / 16;
  const plUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  PL_ASSERT_DEBUG(uiIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiIndex] &= ~(FLAGS_MASK << uiSubIndex);
  m_pEntryFlags[uiIndex] |= (uiFlags << uiSubIndex);
#else
  PL_ASSERT_DEBUG(uiEntryIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiEntryIndex] = uiFlags;
#endif
}

template <typename K, typename H>
PL_FORCE_INLINE bool plHashSetBase<K, H>::IsFreeEntry(plUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
}

template <typename K, typename H>
PL_FORCE_INLINE bool plHashSetBase<K, H>::IsValidEntry(plUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
}

template <typename K, typename H>
PL_FORCE_INLINE bool plHashSetBase<K, H>::IsDeletedEntry(plUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
}

template <typename K, typename H>
PL_FORCE_INLINE void plHashSetBase<K, H>::MarkEntryAsFree(plUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, FREE_ENTRY);
}

template <typename K, typename H>
PL_FORCE_INLINE void plHashSetBase<K, H>::MarkEntryAsValid(plUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, VALID_ENTRY);
}

template <typename K, typename H>
PL_FORCE_INLINE void plHashSetBase<K, H>::MarkEntryAsDeleted(plUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, DELETED_ENTRY);
}


template <typename K, typename H, typename A>
plHashSet<K, H, A>::plHashSet()
  : plHashSetBase<K, H>(A::GetAllocator())
{
}

template <typename K, typename H, typename A>
plHashSet<K, H, A>::plHashSet(plAllocator* pAllocator)
  : plHashSetBase<K, H>(pAllocator)
{
}

template <typename K, typename H, typename A>
plHashSet<K, H, A>::plHashSet(const plHashSet<K, H, A>& other)
  : plHashSetBase<K, H>(other, A::GetAllocator())
{
}

template <typename K, typename H, typename A>
plHashSet<K, H, A>::plHashSet(const plHashSetBase<K, H>& other)
  : plHashSetBase<K, H>(other, A::GetAllocator())
{
}

template <typename K, typename H, typename A>
plHashSet<K, H, A>::plHashSet(plHashSet<K, H, A>&& other)
  : plHashSetBase<K, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename H, typename A>
plHashSet<K, H, A>::plHashSet(plHashSetBase<K, H>&& other)
  : plHashSetBase<K, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename H, typename A>
void plHashSet<K, H, A>::operator=(const plHashSet<K, H, A>& rhs)
{
  plHashSetBase<K, H>::operator=(rhs);
}

template <typename K, typename H, typename A>
void plHashSet<K, H, A>::operator=(const plHashSetBase<K, H>& rhs)
{
  plHashSetBase<K, H>::operator=(rhs);
}

template <typename K, typename H, typename A>
void plHashSet<K, H, A>::operator=(plHashSet<K, H, A>&& rhs)
{
  plHashSetBase<K, H>::operator=(std::move(rhs));
}

template <typename K, typename H, typename A>
void plHashSet<K, H, A>::operator=(plHashSetBase<K, H>&& rhs)
{
  plHashSetBase<K, H>::operator=(std::move(rhs));
}

template <typename KeyType, typename Hasher>
void plHashSetBase<KeyType, Hasher>::Swap(plHashSetBase<KeyType, Hasher>& other)
{
  plMath::Swap(this->m_pEntries, other.m_pEntries);
  plMath::Swap(this->m_pEntryFlags, other.m_pEntryFlags);
  plMath::Swap(this->m_uiCount, other.m_uiCount);
  plMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  plMath::Swap(this->m_pAllocator, other.m_pAllocator);
}
