#include <Foundation/Strings/Implementation/StringBase.h>

namespace plInternal
{
  template <typename T, bool isString>
  struct HashHelperImpl
  {
    static plUInt32 Hash(const T& value);
  };

  template <typename T>
  struct HashHelperImpl<T, true>
  {
    PLASMA_ALWAYS_INLINE static plUInt32 Hash(plStringView sString)
    {
      return plHashingUtils::StringHashTo32(plHashingUtils::StringHash(sString));
    }
  };

  template <typename T, bool isString>
  PLASMA_ALWAYS_INLINE plUInt32 HashHelperImpl<T, isString>::Hash(const T& value)
  {
    PLASMA_CHECK_AT_COMPILETIME_MSG(isString, "plHashHelper is not implemented for the given type.");
    return 0;
  }
} // namespace plInternal

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE plUInt32 plHashHelper<T>::Hash(const U& value)
{
  return plInternal::HashHelperImpl<T, PLASMA_IS_DERIVED_FROM_STATIC(plThisIsAString, T)>::Hash(value);
}

template <typename T>
template <typename U>
PLASMA_ALWAYS_INLINE bool plHashHelper<T>::Equal(const T& a, const U& b)
{
  return a == b;
}



template <>
struct plHashHelper<plUInt32>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plUInt32 value)
  {
    // Knuth: multiplication by the golden ratio will minimize gaps in the hash space.
    // 2654435761U: prime close to 2^32/phi with phi = golden ratio (sqrt(5) - 1) / 2
    return value * 2654435761U;
  }

  PLASMA_ALWAYS_INLINE static bool Equal(plUInt32 a, plUInt32 b) { return a == b; }
};

template <>
struct plHashHelper<plInt32>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plInt32 value) { return plHashHelper<plUInt32>::Hash(plUInt32(value)); }

  PLASMA_ALWAYS_INLINE static bool Equal(plInt32 a, plInt32 b) { return a == b; }
};

template <>
struct plHashHelper<plUInt64>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plUInt64 value)
  {
    // boost::hash_combine.
    plUInt32 a = plUInt32(value >> 32);
    plUInt32 b = plUInt32(value);
    return a ^ (b + 0x9e3779b9 + (a << 6) + (b >> 2));
  }

  PLASMA_ALWAYS_INLINE static bool Equal(plUInt64 a, plUInt64 b) { return a == b; }
};

template <>
struct plHashHelper<plInt64>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plInt64 value) { return plHashHelper<plUInt64>::Hash(plUInt64(value)); }

  PLASMA_ALWAYS_INLINE static bool Equal(plInt64 a, plInt64 b) { return a == b; }
};

template <>
struct plHashHelper<const char*>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const char* szValue)
  {
    return plHashingUtils::StringHashTo32(plHashingUtils::StringHash(szValue));
  }

  PLASMA_ALWAYS_INLINE static bool Equal(const char* a, const char* b) { return plStringUtils::IsEqual(a, b); }
};

template <>
struct plHashHelper<plStringView>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plStringView sValue)
  {
    return plHashingUtils::StringHashTo32(plHashingUtils::StringHash(sValue));
  }

  PLASMA_ALWAYS_INLINE static bool Equal(plStringView a, plStringView b) { return a == b; }
};

template <typename T>
struct plHashHelper<T*>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(T* value)
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
    return plHashHelper<plUInt64>::Hash(reinterpret_cast<plUInt64>(value) >> 4);
#else
    return plHashHelper<plUInt32>::Hash(reinterpret_cast<plUInt32>(value) >> 4);
#endif
  }

  PLASMA_ALWAYS_INLINE static bool Equal(T* a, T* b)
  {
    return a == b;
  }
};

template <size_t N>
constexpr PLASMA_ALWAYS_INLINE plUInt64 plHashingUtils::StringHash(const char (&str)[N], plUInt64 uiSeed)
{
  return xxHash64String(str, uiSeed);
}

PLASMA_ALWAYS_INLINE plUInt64 plHashingUtils::StringHash(plStringView sStr, plUInt64 uiSeed)
{
  return xxHash64String(sStr, uiSeed);
}

constexpr PLASMA_ALWAYS_INLINE plUInt32 plHashingUtils::StringHashTo32(plUInt64 uiHash)
{
  // just throw away the upper bits
  return static_cast<plUInt32>(uiHash);
}

constexpr PLASMA_ALWAYS_INLINE plUInt32 plHashingUtils::CombineHashValues32(plUInt32 ui0, plUInt32 ui1)
{
  // See boost::hash_combine
  return ui0 ^ (ui1 + 0x9e3779b9 + (ui0 << 6) + (ui1 >> 2));
}
