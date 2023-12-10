#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Constants.h>

/// \brief A template interface, that turns any array class into a bitfield.
///
/// This class provides an interface to work with single bits, to store true/false values.
/// The underlying container is configurable, though it must support random access and a 'SetCount' function and it must use elements of type
/// plUInt32. In most cases a dynamic array should be used. For this case the plDynamicBitfield typedef is already available. There is also an
/// plHybridBitfield typedef.
template <class Container>
class plBitfield
{
public:
  plBitfield() = default;

  /// \brief Returns the number of bits that this bitfield stores.
  plUInt32 GetCount() const; // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. This version does NOT initialize new bits!
  template <typename = void>                       // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(plUInt32 uiBitCount); // [tested]

  /// \brief Resizes the Bitfield to hold the given number of bits. If \a bSetNew is true, new bits are set to 1, otherwise they are cleared to 0.
  void SetCount(plUInt32 uiBitCount, bool bSetNew = false); // [tested]

  /// \brief Returns true, if the bitfield does not store any bits.
  bool IsEmpty() const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and any bit is 1.
  bool IsAnyBitSet(plUInt32 uiFirstBit = 0, plUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Returns true, if the bitfield is empty or all bits are set to zero.
  bool IsNoBitSet(plUInt32 uiFirstBit = 0, plUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet(plUInt32 uiFirstBit = 0, plUInt32 uiNumBits = 0xFFFFFFFF) const; // [tested]

  /// \brief Discards all bits and sets count to zero.
  void Clear(); // [tested]

  /// \brief Sets the given bit to 1.
  void SetBit(plUInt32 uiBit); // [tested]

  /// \brief Clears the given bit to 0.
  void ClearBit(plUInt32 uiBit); // [tested]

  /// \brief Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(plUInt32 uiBit, bool bValue); // [tested]

  /// \brief Returns true, if the given bit is set to 1.
  bool IsBitSet(plUInt32 uiBit) const; // [tested]

  /// \brief Clears all bits to 0.
  void ClearAllBits(); // [tested]

  /// \brief Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// \brief Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits); // [tested]

  /// \brief Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits); // [tested]

private:
  plUInt32 GetBitInt(plUInt32 uiBitIndex) const;
  plUInt32 GetBitMask(plUInt32 uiBitIndex) const;

  plUInt32 m_uiCount = 0;
  Container m_Container;
};

/// \brief This should be the main type of bitfield to use, although other internal container types are possible.
using plDynamicBitfield = plBitfield<plDynamicArray<plUInt32>>;

/// \brief An plBitfield that uses a hybrid array as internal container.
template <plUInt32 BITS>
using plHybridBitfield = plBitfield<plHybridArray<plUInt32, (BITS + 31) / 32>>;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <typename T>
class plStaticBitfield
{
public:
  using StorageType = T;
  static constexpr plUInt32 GetStorageTypeBitCount() { return plMath::NumBits<T>(); }

  /// \brief Initializes the bitfield to all zero.
  plStaticBitfield();

  static plStaticBitfield<T> MakeFromMask(StorageType bits);

  /// \brief Returns true, if the bitfield is not zero.
  bool IsAnyBitSet() const; // [tested]

  /// \brief Returns true, if the bitfield is all zero.
  bool IsNoBitSet() const; // [tested]

  /// \brief Returns true, if the bitfield is not empty and all bits are set to one.
  bool AreAllBitsSet() const; // [tested]

  /// \brief Sets the given bit to 1.
  void SetBit(plUInt32 uiBit); // [tested]

  /// \brief Clears the given bit to 0.
  void ClearBit(plUInt32 uiBit); // [tested]

  /// \brief Sets the given bit to 1 or 0 depending on the given value.
  void SetBitValue(plUInt32 uiBit, bool bValue); // [tested]

  /// \brief Returns true, if the given bit is set to 1.
  bool IsBitSet(plUInt32 uiBit) const; // [tested]

  /// \brief Clears all bits to 0. Same as Clear().
  void ClearAllBits(); // [tested]

  /// \brief Sets all bits to 1.
  void SetAllBits(); // [tested]

  /// \brief Sets the range starting at uiFirstBit up to (and including) uiLastBit to 1.
  void SetBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits); // [tested]

  /// \brief Clears the range starting at uiFirstBit up to (and including) uiLastBit to 0.
  void ClearBitRange(plUInt32 uiFirstBit, plUInt32 uiNumBits); // [tested]

  /// \brief Returns the index of the lowest bit that is set. Returns the max index+1 in case no bit is set, at all.
  plUInt32 GetLowestBitSet() const; // [tested]

  /// \brief Returns the index of the highest bit that is set. Returns the max index+1 in case no bit is set, at all.
  plUInt32 GetHighestBitSet() const; // [tested]

  /// \brief Returns the count of how many bits are set in total.
  plUInt32 GetNumBitsSet() const; // [tested]

  /// \brief Returns the raw uint that stores all bits.
  T GetValue() const; // [tested]

  /// \brief Sets the raw uint that stores all bits.
  void SetValue(T value); // [tested]

  /// \brief Modifies \a this to also contain the bits from \a rhs.
  PLASMA_ALWAYS_INLINE void operator|=(const plStaticBitfield<T>& rhs) { m_Storage |= rhs.m_Storage; }

  /// \brief Modifies \a this to only contain the bits that were set in \a this and \a rhs.
  PLASMA_ALWAYS_INLINE void operator&=(const plStaticBitfield<T>& rhs) { m_Storage &= rhs.m_Storage; }

  plResult Serialize(plStreamWriter& inout_writer) const
  {
    inout_writer.WriteVersion(s_Version);
    inout_writer << m_Storage;
    return PLASMA_SUCCESS;
  }

  plResult Deserialize(plStreamReader& inout_reader)
  {
    /*auto version =*/inout_reader.ReadVersion(s_Version);
    inout_reader >> m_Storage;
    return PLASMA_SUCCESS;
  }

private:
  static constexpr plTypeVersion s_Version = 1;

  plStaticBitfield(StorageType initValue)
    : m_Storage(initValue)
  {
  }

  template <typename U>
  friend plStaticBitfield<U> operator|(plStaticBitfield<U> lhs, plStaticBitfield<U> rhs);

  template <typename U>
  friend plStaticBitfield<U> operator&(plStaticBitfield<U> lhs, plStaticBitfield<U> rhs);

  template <typename U>
  friend plStaticBitfield<U> operator^(plStaticBitfield<U> lhs, plStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator==(plStaticBitfield<U> lhs, plStaticBitfield<U> rhs);

  template <typename U>
  friend bool operator!=(plStaticBitfield<U> lhs, plStaticBitfield<U> rhs);

  StorageType m_Storage = 0;
};

template <typename T>
inline plStaticBitfield<T> operator|(plStaticBitfield<T> lhs, plStaticBitfield<T> rhs)
{
  return plStaticBitfield<T>(lhs.m_Storage | rhs.m_Storage);
}

template <typename T>
inline plStaticBitfield<T> operator&(plStaticBitfield<T> lhs, plStaticBitfield<T> rhs)
{
  return plStaticBitfield<T>(lhs.m_Storage & rhs.m_Storage);
}

template <typename T>
inline plStaticBitfield<T> operator^(plStaticBitfield<T> lhs, plStaticBitfield<T> rhs)
{
  return plStaticBitfield<T>(lhs.m_Storage ^ rhs.m_Storage);
}

template <typename T>
inline bool operator==(plStaticBitfield<T> lhs, plStaticBitfield<T> rhs)
{
  return lhs.m_Storage == rhs.m_Storage;
}

template <typename T>
inline bool operator!=(plStaticBitfield<T> lhs, plStaticBitfield<T> rhs)
{
  return lhs.m_Storage != rhs.m_Storage;
}

using plStaticBitfield8 = plStaticBitfield<plUInt8>;
using plStaticBitfield16 = plStaticBitfield<plUInt16>;
using plStaticBitfield32 = plStaticBitfield<plUInt32>;
using plStaticBitfield64 = plStaticBitfield<plUInt64>;

#include <Foundation/Containers/Implementation/Bitfield_inl.h>
