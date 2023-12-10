#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

// Standard operators for overloads of common data types

/// bool versions

inline plStreamWriter& operator<<(plStreamWriter& inout_stream, bool bValue)
{
  plUInt8 uiValue = bValue ? 1 : 0;
  inout_stream.WriteBytes(&uiValue, sizeof(plUInt8)).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, bool& out_bValue)
{
  plUInt8 uiValue = 0;
  PLASMA_VERIFY(inout_stream.ReadBytes(&uiValue, sizeof(plUInt8)) == sizeof(plUInt8), "End of stream reached.");
  out_bValue = (uiValue != 0);
  return inout_stream;
}

/// unsigned int versions

inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plUInt8 uiValue)
{
  inout_stream.WriteBytes(&uiValue, sizeof(plUInt8)).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plUInt8& out_uiValue)
{
  //PLASMA_VERIFY(inout_stream.ReadBytes(&out_uiValue, sizeof(plUInt8)) == sizeof(plUInt8), "End of stream reached.");
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plUInt8* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plUInt8) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plUInt8* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plUInt8) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plUInt16 uiValue)
{
  inout_stream.WriteWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plUInt16& ref_uiValue)
{
  inout_stream.ReadWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plUInt16* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plUInt16) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plUInt16* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plUInt16) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plUInt32 uiValue)
{
  inout_stream.WriteDWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plUInt32& ref_uiValue)
{
  inout_stream.ReadDWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plUInt32* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plUInt32) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plUInt32* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plUInt32) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plUInt64 uiValue)
{
  inout_stream.WriteQWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plUInt64& ref_uiValue)
{
  inout_stream.ReadQWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plUInt64* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plUInt64) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plUInt64* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plUInt64) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}

/// signed int versions

inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plInt8 iValue)
{
  inout_stream.WriteBytes(reinterpret_cast<const plUInt8*>(&iValue), sizeof(plInt8)).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plInt8& ref_iValue)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(reinterpret_cast<plUInt8*>(&ref_iValue), sizeof(plInt8)) == sizeof(plInt8), "End of stream reached.");
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plInt8* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plInt8) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plInt8* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plInt8) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plInt16 iValue)
{
  inout_stream.WriteWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plInt16& ref_iValue)
{
  inout_stream.ReadWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plInt16* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plInt16) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plInt16* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plInt16) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plInt32 iValue)
{
  inout_stream.WriteDWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plInt32& ref_iValue)
{
  inout_stream.ReadDWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plInt32* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plInt32) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plInt32* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plInt32) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


inline plStreamWriter& operator<<(plStreamWriter& inout_stream, plInt64 iValue)
{
  inout_stream.WriteQWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plInt64& ref_iValue)
{
  inout_stream.ReadQWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plInt64* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plInt64) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, plInt64* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plInt64) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


/// float and double versions

inline plStreamWriter& operator<<(plStreamWriter& inout_stream, float fValue)
{
  inout_stream.WriteDWordValue(&fValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, float& ref_fValue)
{
  inout_stream.ReadDWordValue(&ref_fValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const float* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(float) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, float* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(float) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


inline plStreamWriter& operator<<(plStreamWriter& inout_stream, double fValue)
{
  inout_stream.WriteQWordValue(&fValue).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, double& ref_fValue)
{
  inout_stream.ReadQWordValue(&ref_fValue).AssertSuccess();
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const double* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(double) * uiCount);
}

inline plResult DeserializeArray(plStreamReader& inout_stream, double* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(double) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// C-style strings
// No read equivalent for C-style strings (but can be read as plString & plStringBuilder instances)

PLASMA_FOUNDATION_DLL plStreamWriter& operator<<(plStreamWriter& inout_stream, const char* szValue);
PLASMA_FOUNDATION_DLL plStreamWriter& operator<<(plStreamWriter& inout_stream, plStringView sValue);

// plHybridString

template <plUInt16 Size, typename AllocatorWrapper>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plHybridString<Size, AllocatorWrapper>& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
  return inout_stream;
}

template <plUInt16 Size, typename AllocatorWrapper>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plHybridString<Size, AllocatorWrapper>& out_sValue)
{
  plStringBuilder builder;
  inout_stream.ReadString(builder).AssertSuccess();
  out_sValue = std::move(builder);

  return inout_stream;
}

// plStringBuilder

PLASMA_FOUNDATION_DLL plStreamWriter& operator<<(plStreamWriter& inout_stream, const plStringBuilder& sValue);
PLASMA_FOUNDATION_DLL plStreamReader& operator>>(plStreamReader& inout_stream, plStringBuilder& out_sValue);

// plEnum

template <typename T>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plEnum<T>& value)
{
  inout_stream << value.GetValue();

  return inout_stream;
}

template <typename T>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plEnum<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  inout_stream >> storedValue;
  value.SetValue(storedValue);

  return inout_stream;
}

// plBitflags

template <typename T>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plBitflags<T>& value)
{
  inout_stream << value.GetValue();

  return inout_stream;
}

template <typename T>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plBitflags<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  inout_stream >> storedValue;
  value.SetValue(storedValue);

  return inout_stream;
}
