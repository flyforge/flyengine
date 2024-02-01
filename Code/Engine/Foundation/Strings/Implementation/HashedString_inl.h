#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

inline plHashedString::plHashedString(const plHashedString& rhs)
{
  m_Data = rhs.m_Data;

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  // the string has a refcount of at least one (rhs holds a reference), thus it will definitely not get deleted on some other thread
  // therefore we can simply increase the refcount without locking
  m_Data.Value().m_iRefCount.Increment();
#endif
}

PL_FORCE_INLINE plHashedString::plHashedString(plHashedString&& rhs)
{
  m_Data = rhs.m_Data;
  rhs.m_Data = HashedType(); // This leaves the string in an invalid state, all operations will fail except the destructor
}

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
inline plHashedString::~plHashedString()
{
  // Explicit check if data is still valid. It can be invalid if this string has been moved.
  if (m_Data.IsValid())
  {
    // just decrease the refcount of the object that we are set to, it might reach refcount zero, but we don't care about that here
    m_Data.Value().m_iRefCount.Decrement();
  }
}
#endif

inline void plHashedString::operator=(const plHashedString& rhs)
{
  // first increase the other refcount, then decrease ours
  HashedType tmp = rhs.m_Data;

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Increment();

  m_Data.Value().m_iRefCount.Decrement();
#endif

  m_Data = tmp;
}

PL_FORCE_INLINE void plHashedString::operator=(plHashedString&& rhs)
{
#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  m_Data.Value().m_iRefCount.Decrement();
#endif

  m_Data = rhs.m_Data;
  rhs.m_Data = HashedType();
}

template <size_t N>
PL_FORCE_INLINE void plHashedString::Assign(const char (&string)[N])
{
#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  HashedType tmp = m_Data;
#endif
  // this function will already increase the refcount as needed
  m_Data = AddHashedString(string, plHashingUtils::StringHash(string));

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Decrement();
#endif
}

PL_FORCE_INLINE void plHashedString::Assign(plStringView sString)
{
#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  HashedType tmp = m_Data;
#endif
  // this function will already increase the refcount as needed
  m_Data = AddHashedString(sString, plHashingUtils::StringHash(sString));

#if PL_ENABLED(PL_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Decrement();
#endif
}

inline bool plHashedString::operator==(const plHashedString& rhs) const
{
  return m_Data == rhs.m_Data;
}

inline bool plHashedString::operator==(const plTempHashedString& rhs) const
{
  return m_Data.Key() == rhs.m_uiHash;
}

inline bool plHashedString::operator<(const plHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_Data.Key();
}

inline bool plHashedString::operator<(const plTempHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_uiHash;
}

PL_ALWAYS_INLINE const plString& plHashedString::GetString() const
{
  return m_Data.Value().m_sString;
}

PL_ALWAYS_INLINE const char* plHashedString::GetData() const
{
  return m_Data.Value().m_sString.GetData();
}

PL_ALWAYS_INLINE plUInt64 plHashedString::GetHash() const
{
  return m_Data.Key();
}

template <size_t N>
PL_FORCE_INLINE plHashedString plMakeHashedString(const char (&string)[N])
{
  plHashedString sResult;
  sResult.Assign(string);
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

PL_ALWAYS_INLINE plTempHashedString::plTempHashedString()
{
  constexpr plUInt64 uiEmptyHash = plHashingUtils::StringHash("");
  m_uiHash = uiEmptyHash;
}

template <size_t N>
PL_ALWAYS_INLINE plTempHashedString::plTempHashedString(const char (&string)[N])
{
  m_uiHash = plHashingUtils::StringHash<N>(string);
}

PL_ALWAYS_INLINE plTempHashedString::plTempHashedString(plStringView sString)
{
  m_uiHash = plHashingUtils::StringHash(sString);
}

PL_ALWAYS_INLINE plTempHashedString::plTempHashedString(const plTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

PL_ALWAYS_INLINE plTempHashedString::plTempHashedString(const plHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

PL_ALWAYS_INLINE plTempHashedString::plTempHashedString(plUInt64 uiHash)
{
  m_uiHash = uiHash;
}

template <size_t N>
PL_ALWAYS_INLINE void plTempHashedString::operator=(const char (&string)[N])
{
  m_uiHash = plHashingUtils::StringHash<N>(string);
}

PL_ALWAYS_INLINE void plTempHashedString::operator=(plStringView sString)
{
  m_uiHash = plHashingUtils::StringHash(sString);
}

PL_ALWAYS_INLINE void plTempHashedString::operator=(const plTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

PL_ALWAYS_INLINE void plTempHashedString::operator=(const plHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

PL_ALWAYS_INLINE bool plTempHashedString::operator==(const plTempHashedString& rhs) const
{
  return m_uiHash == rhs.m_uiHash;
}

PL_ALWAYS_INLINE bool plTempHashedString::operator<(const plTempHashedString& rhs) const
{
  return m_uiHash < rhs.m_uiHash;
}

PL_ALWAYS_INLINE bool plTempHashedString::IsEmpty() const
{
  constexpr plUInt64 uiEmptyHash = plHashingUtils::StringHash("");
  return m_uiHash == uiEmptyHash;
}

PL_ALWAYS_INLINE void plTempHashedString::Clear()
{
  *this = plTempHashedString();
}

PL_ALWAYS_INLINE plUInt64 plTempHashedString::GetHash() const
{
  return m_uiHash;
}

//////////////////////////////////////////////////////////////////////////

template <>
struct plHashHelper<plHashedString>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(const plHashedString& value)
  {
    return plHashingUtils::StringHashTo32(value.GetHash());
  }

  PL_ALWAYS_INLINE static plUInt32 Hash(const plTempHashedString& value)
  {
    return plHashingUtils::StringHashTo32(value.GetHash());
  }

  PL_ALWAYS_INLINE static bool Equal(const plHashedString& a, const plHashedString& b) { return a == b; }

  PL_ALWAYS_INLINE static bool Equal(const plHashedString& a, const plTempHashedString& b) { return a == b; }
};

template <>
struct plHashHelper<plTempHashedString>
{
  PL_ALWAYS_INLINE static plUInt32 Hash(const plTempHashedString& value)
  {
    return plHashingUtils::StringHashTo32(value.GetHash());
  }

  PL_ALWAYS_INLINE static bool Equal(const plTempHashedString& a, const plTempHashedString& b) { return a == b; }
};
