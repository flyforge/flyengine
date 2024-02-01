
#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

class plStreamReader;
class plStreamWriter;

/// \brief This data type is the abstraction for 128-bit Uuid (also known as GUID) instances.
class PL_FOUNDATION_DLL plUuid
{
public:
  PL_DECLARE_POD_TYPE();

  /// \brief Default constructor. Constructed Uuid will be invalid.
  PL_ALWAYS_INLINE plUuid(); // [tested]

  /// \brief Constructs the Uuid from existing values
  PL_ALWAYS_INLINE plUuid(plUInt64 uiLow, plUInt64 uiHigh)
  {
    m_uiLow = uiLow;
    m_uiHigh = uiHigh;
  }

  /// \brief Comparison operator. [tested]
  PL_ALWAYS_INLINE bool operator==(const plUuid& other) const;

  /// \brief Comparison operator. [tested]
  PL_ALWAYS_INLINE bool operator!=(const plUuid& other) const;

  /// \brief Comparison operator.
  PL_ALWAYS_INLINE bool operator<(const plUuid& other) const;

  /// \brief Returns true if this is a valid Uuid.
  PL_ALWAYS_INLINE bool IsValid() const;

  /// \brief Returns an invalid UUID.
  [[nodiscard]] PL_ALWAYS_INLINE static plUuid MakeInvalid() { return plUuid(0, 0); }

  /// \brief Returns a new Uuid.
  [[nodiscard]] static plUuid MakeUuid();

  /// \brief Returns the internal 128 Bit of data
  void GetValues(plUInt64& ref_uiLow, plUInt64& ref_uiHigh) const
  {
    ref_uiHigh = m_uiHigh;
    ref_uiLow = m_uiLow;
  }

  /// \brief Creates a uuid from a string. The result is always the same for the same string.
  [[nodiscard]] static plUuid MakeStableUuidFromString(plStringView sString);

  /// \brief Creates a uuid from an integer. The result is always the same for the same input.
  [[nodiscard]] static plUuid MakeStableUuidFromInt(plInt64 iInt);

  /// \brief Adds the given seed value to this guid, creating a new guid. The process is reversible.
  PL_ALWAYS_INLINE void CombineWithSeed(const plUuid& seed);

  /// \brief Subtracts the given seed from this guid, restoring the original guid.
  PL_ALWAYS_INLINE void RevertCombinationWithSeed(const plUuid& seed);

  /// \brief Combines two guids using hashing, irreversible and order dependent.
  PL_ALWAYS_INLINE void HashCombine(const plUuid& hash);

private:
  friend PL_FOUNDATION_DLL_FRIEND void operator>>(plStreamReader& inout_stream, plUuid& ref_value);
  friend PL_FOUNDATION_DLL_FRIEND void operator<<(plStreamWriter& inout_stream, const plUuid& value);

  plUInt64 m_uiHigh;
  plUInt64 m_uiLow;
};

#include <Foundation/Types/Implementation/Uuid_inl.h>
