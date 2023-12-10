#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/TypeTraits.h>

#define PLASMA_DECLARE_VARIANCE_HASH_HELPER(TYPE)                        \
  template <>                                                        \
  struct plHashHelper<TYPE>                                          \
  {                                                                  \
    PLASMA_ALWAYS_INLINE static plUInt32 Hash(const TYPE& value)         \
    {                                                                \
      return plHashingUtils::xxHash32(&value, sizeof(TYPE));         \
    }                                                                \
    PLASMA_ALWAYS_INLINE static bool Equal(const TYPE& a, const TYPE& b) \
    {                                                                \
      return a == b;                                                 \
    }                                                                \
  };

struct PLASMA_FOUNDATION_DLL plVarianceTypeBase
{
  PLASMA_DECLARE_POD_TYPE();

  float m_fVariance = 0;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plVarianceTypeBase);

struct PLASMA_FOUNDATION_DLL plVarianceTypeFloat : public plVarianceTypeBase
{
  PLASMA_DECLARE_POD_TYPE();
  bool operator==(const plVarianceTypeFloat& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const plVarianceTypeFloat& rhs) const
  {
    return !(*this == rhs);
  }
  float m_Value = 0;
};

PLASMA_DECLARE_VARIANCE_HASH_HELPER(plVarianceTypeFloat);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plVarianceTypeFloat);
PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plVarianceTypeFloat);

struct PLASMA_FOUNDATION_DLL plVarianceTypeTime : public plVarianceTypeBase
{
  PLASMA_DECLARE_POD_TYPE();
  bool operator==(const plVarianceTypeTime& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const plVarianceTypeTime& rhs) const
  {
    return !(*this == rhs);
  }
  plTime m_Value;
};

PLASMA_DECLARE_VARIANCE_HASH_HELPER(plVarianceTypeTime);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plVarianceTypeTime);
PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plVarianceTypeTime);

struct PLASMA_FOUNDATION_DLL plVarianceTypeAngle : public plVarianceTypeBase
{
  PLASMA_DECLARE_POD_TYPE();
  bool operator==(const plVarianceTypeAngle& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const plVarianceTypeAngle& rhs) const
  {
    return !(*this == rhs);
  }
  plAngle m_Value;
};

PLASMA_DECLARE_VARIANCE_HASH_HELPER(plVarianceTypeAngle);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plVarianceTypeAngle);
PLASMA_DECLARE_CUSTOM_VARIANT_TYPE(plVarianceTypeAngle);
