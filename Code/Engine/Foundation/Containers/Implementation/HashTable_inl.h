
/// \brief Value used by containers for indices to indicate an invalid index.
#ifndef plInvalidIndex
#  define plInvalidIndex 0xFFFFFFFF
#endif

// ***** Const Iterator *****

template <typename K, typename V, typename H>
plHashTableBase<K, V, H>::ConstIterator::ConstIterator(const plHashTableBase<K, V, H>& hashTable)
  : m_pHashTable(&hashTable)
{
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::ConstIterator::SetToBegin()
{
  if (m_pHashTable->IsEmpty())
  {
    m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
    return;
  }
  while (!m_pHashTable->IsValidEntry(m_uiCurrentIndex))
  {
    ++m_uiCurrentIndex;
  }
}

template <typename K, typename V, typename H>
inline void plHashTableBase<K, V, H>::ConstIterator::SetToEnd()
{
  m_uiCurrentCount = m_pHashTable->m_uiCount;
  m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
}


template <typename K, typename V, typename H>
PL_FORCE_INLINE bool plHashTableBase<K, V, H>::ConstIterator::IsValid() const
{
  return m_uiCurrentCount < m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
PL_FORCE_INLINE bool plHashTableBase<K, V, H>::ConstIterator::operator==(const typename plHashTableBase<K, V, H>::ConstIterator& rhs) const
{
  return m_uiCurrentIndex == rhs.m_uiCurrentIndex && m_pHashTable->m_pEntries == rhs.m_pHashTable->m_pEntries;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE const K& plHashTableBase<K, V, H>::ConstIterator::Key() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].key;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE const V& plHashTableBase<K, V, H>::ConstIterator::Value() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].value;
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::ConstIterator::Next()
{
  // if we already iterated over the amount of valid elements that the hash-table stores, early out
  if (m_uiCurrentCount >= m_pHashTable->m_uiCount)
    return;

  // increase the counter of how many elements we have seen
  ++m_uiCurrentCount;
  // increase the index of the element to look at
  ++m_uiCurrentIndex;

  // check that we don't leave the valid range of element indices
  while (m_uiCurrentIndex < m_pHashTable->m_uiCapacity)
  {
    if (m_pHashTable->IsValidEntry(m_uiCurrentIndex))
      return;

    ++m_uiCurrentIndex;
  }

  // if we fell through this loop, we reached the end of all elements in the container
  // set the m_uiCurrentCount to maximum, to enable early-out in the future and to make 'IsValid' return 'false'
  m_uiCurrentCount = m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE void plHashTableBase<K, V, H>::ConstIterator::operator++()
{
  Next();
}


// ***** Iterator *****

template <typename K, typename V, typename H>
plHashTableBase<K, V, H>::Iterator::Iterator(const plHashTableBase<K, V, H>& hashTable)
  : ConstIterator(hashTable)
{
}

template <typename K, typename V, typename H>
plHashTableBase<K, V, H>::Iterator::Iterator(const typename plHashTableBase<K, V, H>::Iterator& rhs)
  : ConstIterator(*rhs.m_pHashTable)
{
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE void plHashTableBase<K, V, H>::Iterator::operator=(const Iterator& rhs) // [tested]
{
  this->m_pHashTable = rhs.m_pHashTable;
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
PL_FORCE_INLINE V& plHashTableBase<K, V, H>::Iterator::Value()
{
  return this->m_pHashTable->m_pEntries[this->m_uiCurrentIndex].value;
}


// ***** plHashTableBase *****

template <typename K, typename V, typename H>
plHashTableBase<K, V, H>::plHashTableBase(plAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;
}

template <typename K, typename V, typename H>
plHashTableBase<K, V, H>::plHashTableBase(const plHashTableBase<K, V, H>& other, plAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename K, typename V, typename H>
plHashTableBase<K, V, H>::plHashTableBase(plHashTableBase<K, V, H>&& other, plAllocator* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename K, typename V, typename H>
plHashTableBase<K, V, H>::~plHashTableBase()
{
  Clear();
  PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  PL_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
  m_uiCapacity = 0;
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::operator=(const plHashTableBase<K, V, H>& rhs)
{
  Clear();
  Reserve(rhs.GetCount());

  plUInt32 uiCopied = 0;
  for (plUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
  {
    if (rhs.IsValidEntry(i))
    {
      Insert(rhs.m_pEntries[i].key, rhs.m_pEntries[i].value);
      ++uiCopied;
    }
  }
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::operator=(plHashTableBase<K, V, H>&& rhs)
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
        Insert(std::move(rhs.m_pEntries[i].key), std::move(rhs.m_pEntries[i].value));
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

template <typename K, typename V, typename H>
bool plHashTableBase<K, V, H>::operator==(const plHashTableBase<K, V, H>& rhs) const
{
  if (m_uiCount != rhs.m_uiCount)
    return false;

  plUInt32 uiCompared = 0;
  for (plUInt32 i = 0; uiCompared < m_uiCount; ++i)
  {
    if (IsValidEntry(i))
    {
      const V* pRhsValue = nullptr;
      if (!rhs.TryGetValue(m_pEntries[i].key, pRhsValue))
        return false;

      if (m_pEntries[i].value != *pRhsValue)
        return false;

      ++uiCompared;
    }
  }

  return true;
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::Reserve(plUInt32 uiCapacity)
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

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::Compact()
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
    const plUInt32 uiNewCapacity = plMath::PowerOfTwo_Ceil(m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE plUInt32 plHashTableBase<K, V, H>::GetCount() const
{
  return m_uiCount;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE bool plHashTableBase<K, V, H>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::Clear()
{
  for (plUInt32 i = 0; i < m_uiCapacity; ++i)
  {
    if (IsValidEntry(i))
    {
      plMemoryUtils::Destruct(&m_pEntries[i].key, 1);
      plMemoryUtils::Destruct(&m_pEntries[i].value, 1);
    }
  }

  plMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());
  m_uiCount = 0;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType, typename CompatibleValueType>
bool plHashTableBase<K, V, H>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value, V* out_pOldValue /*= nullptr*/)
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
    else if (H::Equal(m_pEntries[uiIndex].key, key))
    {
      if (out_pOldValue != nullptr)
        *out_pOldValue = std::move(m_pEntries[uiIndex].value);

      m_pEntries[uiIndex].value = std::forward<CompatibleValueType>(value); // Either move or copy assignment.
      return true;
    }
    ++uiIndex;
    if (uiIndex == m_uiCapacity)
      uiIndex = 0;

    ++uiCounter;
  }

  // new entry
  uiIndex = uiDeletedIndex != plInvalidIndex ? uiDeletedIndex : uiIndex;

  // Both constructions might either be a move or a copy.
  plMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].key, std::forward<CompatibleKeyType>(key));
  plMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].value, std::forward<CompatibleValueType>(value));

  MarkEntryAsValid(uiIndex);
  ++m_uiCount;

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
bool plHashTableBase<K, V, H>::Remove(const CompatibleKeyType& key, V* out_pOldValue /*= nullptr*/)
{
  plUInt32 uiIndex = FindEntry(key);
  if (uiIndex != plInvalidIndex)
  {
    if (out_pOldValue != nullptr)
      *out_pOldValue = std::move(m_pEntries[uiIndex].value);

    RemoveInternal(uiIndex);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
typename plHashTableBase<K, V, H>::Iterator plHashTableBase<K, V, H>::Remove(const typename plHashTableBase<K, V, H>::Iterator& pos)
{
  PL_ASSERT_DEBUG(pos.m_pHashTable == this, "Iterator from wrong hashtable");
  Iterator it = pos;
  plUInt32 uiIndex = pos.m_uiCurrentIndex;
  ++it;
  --it.m_uiCurrentCount;
  RemoveInternal(uiIndex);
  return it;
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::RemoveInternal(plUInt32 uiIndex)
{
  plMemoryUtils::Destruct(&m_pEntries[uiIndex].key, 1);
  plMemoryUtils::Destruct(&m_pEntries[uiIndex].value, 1);

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

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool plHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V& out_value) const
{
  plUInt32 uiIndex = FindEntry(key);
  if (uiIndex != plInvalidIndex)
  {
    PL_ASSERT_DEBUG(m_pEntries != nullptr, "No entries present"); // To fix static analysis
    out_value = m_pEntries[uiIndex].value;
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool plHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, const V*& out_pValue) const
{
  plUInt32 uiIndex = FindEntry(key);
  if (uiIndex != plInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    PL_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool plHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V*& out_pValue) const
{
  plUInt32 uiIndex = FindEntry(key);
  if (uiIndex != plInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    PL_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename plHashTableBase<K, V, H>::ConstIterator plHashTableBase<K, V, H>::Find(const CompatibleKeyType& key) const
{
  plUInt32 uiIndex = FindEntry(key);
  if (uiIndex == plInvalidIndex)
  {
    return GetEndIterator();
  }

  ConstIterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0

  return it;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename plHashTableBase<K, V, H>::Iterator plHashTableBase<K, V, H>::Find(const CompatibleKeyType& key)
{
  plUInt32 uiIndex = FindEntry(key);
  if (uiIndex == plInvalidIndex)
  {
    return GetEndIterator();
  }

  Iterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0
  return it;
}


template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline const V* plHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key) const
{
  plUInt32 uiIndex = FindEntry(key);
  return (uiIndex != plInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline V* plHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key)
{
  plUInt32 uiIndex = FindEntry(key);
  return (uiIndex != plInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
inline V& plHashTableBase<K, V, H>::operator[](const K& key)
{
  return FindOrAdd(key, nullptr);
}

template <typename K, typename V, typename H>
V& plHashTableBase<K, V, H>::FindOrAdd(const K& key, bool* out_pExisted)
{
  const plUInt32 uiHash = H::Hash(key);
  plUInt32 uiIndex = FindEntry(uiHash, key);

  if (out_pExisted)
  {
    *out_pExisted = uiIndex != plInvalidIndex;
  }

  if (uiIndex == plInvalidIndex)
  {
    Reserve(m_uiCount + 1);

    // search for suitable insertion index again, table might have been resized
    uiIndex = uiHash & (m_uiCapacity - 1);
    while (IsValidEntry(uiIndex))
    {
      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;
    }

    // new entry
    plMemoryUtils::CopyConstruct(&m_pEntries[uiIndex].key, key, 1);
    plMemoryUtils::Construct<ConstructAll>(&m_pEntries[uiIndex].value, 1);
    MarkEntryAsValid(uiIndex);
    ++m_uiCount;
  }

  PL_ASSERT_DEBUG(m_pEntries != nullptr, "Entries should be present");
  return m_pEntries[uiIndex].value;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
PL_FORCE_INLINE bool plHashTableBase<K, V, H>::Contains(const CompatibleKeyType& key) const
{
  return FindEntry(key) != plInvalidIndex;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE typename plHashTableBase<K, V, H>::Iterator plHashTableBase<K, V, H>::GetIterator()
{
  Iterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE typename plHashTableBase<K, V, H>::Iterator plHashTableBase<K, V, H>::GetEndIterator()
{
  Iterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE typename plHashTableBase<K, V, H>::ConstIterator plHashTableBase<K, V, H>::GetIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE typename plHashTableBase<K, V, H>::ConstIterator plHashTableBase<K, V, H>::GetEndIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE plAllocator* plHashTableBase<K, V, H>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename K, typename V, typename H>
plUInt64 plHashTableBase<K, V, H>::GetHeapMemoryUsage() const
{
  return ((plUInt64)m_uiCapacity * sizeof(Entry)) + (sizeof(plUInt32) * (plUInt64)GetFlagsCapacity());
}

// private methods
template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::SetCapacity(plUInt32 uiCapacity)
{
  PL_ASSERT_DEBUG(plMath::IsPowerOf2(uiCapacity), "uiCapacity must be a power of two to avoid modulo during lookup.");
  const plUInt32 uiOldCapacity = m_uiCapacity;
  m_uiCapacity = uiCapacity;

  Entry* pOldEntries = m_pEntries;
  plUInt32* pOldEntryFlags = m_pEntryFlags;

  m_pEntries = PL_NEW_RAW_BUFFER(m_pAllocator, Entry, m_uiCapacity);
  m_pEntryFlags = PL_NEW_RAW_BUFFER(m_pAllocator, plUInt32, GetFlagsCapacity());
  plMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());

  m_uiCount = 0;
  for (plUInt32 i = 0; i < uiOldCapacity; ++i)
  {
    if (GetFlags(pOldEntryFlags, i) == VALID_ENTRY)
    {
      PL_VERIFY(!Insert(std::move(pOldEntries[i].key), std::move(pOldEntries[i].value)), "Implementation error");

      plMemoryUtils::Destruct(&pOldEntries[i].key, 1);
      plMemoryUtils::Destruct(&pOldEntries[i].value, 1);
    }
  }

  PL_DELETE_RAW_BUFFER(m_pAllocator, pOldEntries);
  PL_DELETE_RAW_BUFFER(m_pAllocator, pOldEntryFlags);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
PL_ALWAYS_INLINE plUInt32 plHashTableBase<K, V, H>::FindEntry(const CompatibleKeyType& key) const
{
  return FindEntry(H::Hash(key), key);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline plUInt32 plHashTableBase<K, V, H>::FindEntry(plUInt32 uiHash, const CompatibleKeyType& key) const
{
  if (m_uiCapacity > 0)
  {
    plUInt32 uiIndex = uiHash & (m_uiCapacity - 1);
    plUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsValidEntry(uiIndex) && H::Equal(m_pEntries[uiIndex].key, key))
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

#define PL_HASHTABLE_USE_BITFLAGS PL_ON

template <typename K, typename V, typename H>
PL_FORCE_INLINE plUInt32 plHashTableBase<K, V, H>::GetFlagsCapacity() const
{
#if PL_ENABLED(PL_HASHTABLE_USE_BITFLAGS)
  return (m_uiCapacity + 15) / 16;
#else
  return m_uiCapacity;
#endif
}

template <typename K, typename V, typename H>
PL_ALWAYS_INLINE plUInt32 plHashTableBase<K, V, H>::GetFlags(plUInt32* pFlags, plUInt32 uiEntryIndex) const
{
#if PL_ENABLED(PL_HASHTABLE_USE_BITFLAGS)
  const plUInt32 uiIndex = uiEntryIndex / 16;
  const plUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
#else
  return pFlags[uiEntryIndex] & FLAGS_MASK;
#endif
}

template <typename K, typename V, typename H>
void plHashTableBase<K, V, H>::SetFlags(plUInt32 uiEntryIndex, plUInt32 uiFlags)
{
#if PL_ENABLED(PL_HASHTABLE_USE_BITFLAGS)
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

template <typename K, typename V, typename H>
PL_FORCE_INLINE bool plHashTableBase<K, V, H>::IsFreeEntry(plUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
}

template <typename K, typename V, typename H>
PL_FORCE_INLINE bool plHashTableBase<K, V, H>::IsValidEntry(plUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
}

template <typename K, typename V, typename H>
PL_FORCE_INLINE bool plHashTableBase<K, V, H>::IsDeletedEntry(plUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
}

template <typename K, typename V, typename H>
PL_FORCE_INLINE void plHashTableBase<K, V, H>::MarkEntryAsFree(plUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, FREE_ENTRY);
}

template <typename K, typename V, typename H>
PL_FORCE_INLINE void plHashTableBase<K, V, H>::MarkEntryAsValid(plUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, VALID_ENTRY);
}

template <typename K, typename V, typename H>
PL_FORCE_INLINE void plHashTableBase<K, V, H>::MarkEntryAsDeleted(plUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, DELETED_ENTRY);
}


template <typename K, typename V, typename H, typename A>
plHashTable<K, V, H, A>::plHashTable()
  : plHashTableBase<K, V, H>(A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
plHashTable<K, V, H, A>::plHashTable(plAllocator* pAllocator)
  : plHashTableBase<K, V, H>(pAllocator)
{
}

template <typename K, typename V, typename H, typename A>
plHashTable<K, V, H, A>::plHashTable(const plHashTable<K, V, H, A>& other)
  : plHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
plHashTable<K, V, H, A>::plHashTable(const plHashTableBase<K, V, H>& other)
  : plHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
plHashTable<K, V, H, A>::plHashTable(plHashTable<K, V, H, A>&& other)
  : plHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
plHashTable<K, V, H, A>::plHashTable(plHashTableBase<K, V, H>&& other)
  : plHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
void plHashTable<K, V, H, A>::operator=(const plHashTable<K, V, H, A>& rhs)
{
  plHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void plHashTable<K, V, H, A>::operator=(const plHashTableBase<K, V, H>& rhs)
{
  plHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void plHashTable<K, V, H, A>::operator=(plHashTable<K, V, H, A>&& rhs)
{
  plHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename K, typename V, typename H, typename A>
void plHashTable<K, V, H, A>::operator=(plHashTableBase<K, V, H>&& rhs)
{
  plHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename KeyType, typename ValueType, typename Hasher>
void plHashTableBase<KeyType, ValueType, Hasher>::Swap(plHashTableBase<KeyType, ValueType, Hasher>& other)
{
  plMath::Swap(this->m_pEntries, other.m_pEntries);
  plMath::Swap(this->m_pEntryFlags, other.m_pEntryFlags);
  plMath::Swap(this->m_uiCount, other.m_uiCount);
  plMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  plMath::Swap(this->m_pAllocator, other.m_pAllocator);
}
