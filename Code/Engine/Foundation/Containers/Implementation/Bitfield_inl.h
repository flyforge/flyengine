#pragma once

template <class Container>
PL_ALWAYS_INLINE plUInt32 plBitfield<Container>::GetBitInt(plUInt32 uiBitIndex) const
{
  return (uiBitIndex >> 5); // div 32
}

template <class Container>
PL_ALWAYS_INLINE plUInt32 plBitfield<Container>::GetBitMask(plUInt32 uiBitIndex) const
{
  return 1 << (uiBitIndex & 0x1F); // modulo 32, shifted to bit position
}

template <class Container>
PL_ALWAYS_INLINE plUInt32 plBitfield<Container>::GetCount() const
{
  return m_uiCount;
}

template <class Container>
template <typename> // Second template needed so that the compiler only instantiates it when called. Needed to prevent errors with containers that do
                    // not support this.
void plBitfield<Container>::SetCountUninitialized(plUInt32 uiBitCount)
{
  const plUInt32 uiInts = (uiBitCount + 31) >> 5;
  m_Container.SetCountUninitialized(uiInts);

  m_uiCount = uiBitCount;
}

template <class Container>
void plBitfield<Container>::SetCount(plUInt32 uiBitCount, bool bSetNew)
{
  if (m_uiCount == uiBitCount)
    return;

  const plUInt32 uiOldBits = m_uiCount;

  SetCountUninitialized(uiBitCount);

  // if there are new bits, initialize them
  if (uiBitCount > uiOldBits)
  {
    if (bSetNew)
      SetBitRange(uiOldBits, uiBitCount - uiOldBits);
    else
      ClearBitRange(uiOldBits, uiBitCount - uiOldBits);
  }
}

template <class Container>
PL_ALWAYS_INLINE bool plBitfield<Container>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <class Container>
bool plBitfield<Container>::IsAnyBitSet(plUInt32 uiFirstBit /*= 0*/, plUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  PL_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const plUInt32 uiLastBit = plMath::Min<plUInt32>(uiFirstBit + uiNumBits, m_uiCount - 1);

  const plUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const plUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (plUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }
  else
  {
    const plUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const plUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (plUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }

    // check the bits in the ints in between with one operation
    for (plUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if ((m_Container[i] & 0xFFFFFFFF) != 0)
        return true;
    }

    // check the bits in the last int individually
    for (plUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (IsBitSet(i))
        return true;
    }
  }

  return false;
}

template <class Container>
PL_ALWAYS_INLINE bool plBitfield<Container>::IsNoBitSet(plUInt32 uiFirstBit /*= 0*/, plUInt32 uiLastBit /*= 0xFFFFFFFF*/) const
{
  return !IsAnyBitSet(uiFirstBit, uiLastBit);
}

template <class Container>
bool plBitfield<Container>::AreAllBitsSet(plUInt32 uiFirstBit /*= 0*/, plUInt32 uiNumBits /*= 0xFFFFFFFF*/) const
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return false;

  PL_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const plUInt32 uiLastBit = plMath::Min<plUInt32>(uiFirstBit + uiNumBits, m_uiCount - 1);

  const plUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const plUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (plUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }
  else
  {
    const plUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
    const plUInt32 uiPrevIntBit = uiLastInt * 32;

    // check the bits in the first int individually
    for (plUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }

    // check the bits in the ints in between with one operation
    for (plUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    {
      if (m_Container[i] != 0xFFFFFFFF)
        return false;
    }

    // check the bits in the last int individually
    for (plUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    {
      if (!IsBitSet(i))
        return false;
    }
  }

  return true;
}

template <class Container>
PL_ALWAYS_INLINE void plBitfield<Container>::Clear()
{
  m_uiCount = 0;
  m_Container.Clear();
}

template <class Container>
void plBitfield<Container>::SetBit(plUInt32 uiBit)
{
  PL_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] |= GetBitMask(uiBit);
}

template <class Container>
void plBitfield<Container>::ClearBit(plUInt32 uiBit)
{
  PL_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  m_Container[GetBitInt(uiBit)] &= ~GetBitMask(uiBit);
}

template <class Container>
PL_ALWAYS_INLINE void plBitfield<Container>::SetBitValue(plUInt32 uiBit, bool bValue)
{
  if (bValue)
  {
    SetBit(uiBit);
  }
  else
  {
    ClearBit(uiBit);
  }
}

template <class Container>
bool plBitfield<Container>::IsBitSet(plUInt32 uiBit) const
{
  PL_ASSERT_DEBUG(uiBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, m_uiCount);

  return (m_Container[GetBitInt(uiBit)] & GetBitMask(uiBit)) != 0;
}

template <class Container>
void plBitfield<Container>::ClearAllBits()
{
  for (plUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0;
}

template <class Container>
void plBitfield<Container>::SetAllBits()
{
  for (plUInt32 i = 0; i < m_Container.GetCount(); ++i)
    m_Container[i] = 0xFFFFFFFF;
}

template <class Container>
void plBitfield<Container>::SetBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  PL_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const plUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const plUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const plUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (plUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      SetBit(i);

    return;
  }

  const plUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const plUInt32 uiPrevIntBit = uiLastInt * 32;

  // set the bits in the first int individually
  for (plUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    SetBit(i);

  // set the bits in the ints in between with one operation
  for (plUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    m_Container[i] = 0xFFFFFFFF;

  // set the bits in the last int individually
  for (plUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    SetBit(i);
}

template <class Container>
void plBitfield<Container>::ClearBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits)
{
  if (m_uiCount == 0 || uiNumBits == 0)
    return;

  PL_ASSERT_DEBUG(uiFirstBit < m_uiCount, "Cannot access bit {0}, the bitfield only has {1} bits.", uiFirstBit, m_uiCount);

  const plUInt32 uiLastBit = uiFirstBit + uiNumBits - 1;

  const plUInt32 uiFirstInt = GetBitInt(uiFirstBit);
  const plUInt32 uiLastInt = GetBitInt(uiLastBit);

  // all within the same int
  if (uiFirstInt == uiLastInt)
  {
    for (plUInt32 i = uiFirstBit; i <= uiLastBit; ++i)
      ClearBit(i);

    return;
  }

  const plUInt32 uiNextIntBit = (uiFirstInt + 1) * 32;
  const plUInt32 uiPrevIntBit = uiLastInt * 32;

  // set the bits in the first int individually
  for (plUInt32 i = uiFirstBit; i < uiNextIntBit; ++i)
    ClearBit(i);

  // set the bits in the ints in between with one operation
  for (plUInt32 i = uiFirstInt + 1; i < uiLastInt; ++i)
    m_Container[i] = 0;

  // set the bits in the last int individually
  for (plUInt32 i = uiPrevIntBit; i <= uiLastBit; ++i)
    ClearBit(i);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
PL_ALWAYS_INLINE plStaticBitfield<T>::plStaticBitfield()
{
  static_assert(std::is_unsigned<T>::value, "Storage type must be unsigned");
}

template <typename T>
PL_ALWAYS_INLINE plStaticBitfield<T> plStaticBitfield<T>::MakeFromMask(StorageType bits)
{
  return plStaticBitfield<T>(bits);
}

template <typename T>
PL_ALWAYS_INLINE bool plStaticBitfield<T>::IsAnyBitSet() const
{
  return m_Storage != 0;
}

template <typename T>
PL_ALWAYS_INLINE bool plStaticBitfield<T>::IsNoBitSet() const
{
  return m_Storage == 0;
}

template <typename T>
bool plStaticBitfield<T>::AreAllBitsSet() const
{
  const T inv = ~m_Storage;
  return inv == 0;
}

template <typename T>
void plStaticBitfield<T>::ClearBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits)
{
  PL_ASSERT_DEBUG(uiFirstBit < GetStorageTypeBitCount(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetStorageTypeBitCount());

  T mask = (uiNumBits / 8 >= sizeof(T)) ? (~static_cast<T>(0)) : ((static_cast<T>(1) << uiNumBits) - 1);
  mask <<= uiFirstBit;
  mask = ~mask;
  m_Storage &= mask;
}

template <typename T>
void plStaticBitfield<T>::SetBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits)
{
  PL_ASSERT_DEBUG(uiFirstBit < GetStorageTypeBitCount(), "Cannot access first bit {0}, the bitfield only has {1} bits.", uiFirstBit, GetStorageTypeBitCount());

  T mask = (uiNumBits / 8 >= sizeof(T)) ? (~static_cast<T>(0)) : ((static_cast<T>(1) << uiNumBits) - 1);
  mask <<= uiFirstBit;
  m_Storage |= mask;
}

template <typename T>
PL_ALWAYS_INLINE plUInt32 plStaticBitfield<T>::GetNumBitsSet() const
{
  return plMath::CountBits(m_Storage);
}

template <typename T>
PL_ALWAYS_INLINE plUInt32 plStaticBitfield<T>::GetHighestBitSet() const
{
  return m_Storage == 0 ? GetStorageTypeBitCount() : plMath::FirstBitHigh(m_Storage);
}

template <typename T>
PL_ALWAYS_INLINE plUInt32 plStaticBitfield<T>::GetLowestBitSet() const
{
  return m_Storage == 0 ? GetStorageTypeBitCount() : plMath::FirstBitLow(m_Storage);
}

template <typename T>
PL_ALWAYS_INLINE void plStaticBitfield<T>::SetAllBits()
{
  m_Storage = plMath::MaxValue<T>(); // possible because we assert that T is unsigned
}

template <typename T>
PL_ALWAYS_INLINE void plStaticBitfield<T>::ClearAllBits()
{
  m_Storage = 0;
}

template <typename T>
PL_ALWAYS_INLINE bool plStaticBitfield<T>::IsBitSet(plUInt32 uiBit) const
{
  PL_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  return (m_Storage & (static_cast<T>(1u) << uiBit)) != 0;
}

template <typename T>
PL_ALWAYS_INLINE void plStaticBitfield<T>::ClearBit(plUInt32 uiBit)
{
  PL_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  m_Storage &= ~(static_cast<T>(1u) << uiBit);
}

template <typename T>
PL_ALWAYS_INLINE void plStaticBitfield<T>::SetBitValue(plUInt32 uiBit, bool bValue)
{
  if (bValue)
  {
    SetBit(uiBit);
  }
  else
  {
    ClearBit(uiBit);
  }
}

template <typename T>
PL_ALWAYS_INLINE void plStaticBitfield<T>::SetBit(plUInt32 uiBit)
{
  PL_ASSERT_DEBUG(uiBit < GetStorageTypeBitCount(), "Cannot access bit {0}, the bitfield only has {1} bits.", uiBit, GetStorageTypeBitCount());

  m_Storage |= static_cast<T>(1u) << uiBit;
}

template <typename T>
PL_ALWAYS_INLINE void plStaticBitfield<T>::SetValue(T value)
{
  m_Storage = value;
}

template <typename T>
PL_ALWAYS_INLINE T plStaticBitfield<T>::GetValue() const
{
  return m_Storage;
}
