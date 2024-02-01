#pragma once

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  Clear();
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const plHybridStringBase& rhs, plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(plHybridStringBase&& rhs, plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  operator=(std::move(rhs));
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const char* rhs, plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const wchar_t* rhs, plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const plStringView& rhs, plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::~plHybridStringBase() = default;

template <plUInt16 Size>
void plHybridStringBase<Size>::Clear()
{
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

template <plUInt16 Size>
PL_ALWAYS_INLINE const char* plHybridStringBase<Size>::GetData() const
{
  PL_ASSERT_DEBUG(!m_Data.IsEmpty(), "plHybridString has been corrupted, the array can never be empty. This can happen when you access a "
                                     "string that was previously std::move'd into another string.");

  return &m_Data[0];
}

template <plUInt16 Size>
PL_ALWAYS_INLINE plUInt32 plHybridStringBase<Size>::GetElementCount() const
{
  return m_Data.GetCount() - 1;
}

template <plUInt16 Size>
PL_ALWAYS_INLINE plUInt32 plHybridStringBase<Size>::GetCharacterCount() const
{
  return plStringUtils::GetCharacterCount(GetData());
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const char* szString)
{
  plUInt32 uiElementCount = plStringUtils::GetStringElementCount(szString);

  if (szString + uiElementCount < m_Data.GetData() || szString >= m_Data.GetData() + m_Data.GetCount())
  {
    // source string is outside our own memory, so no overlapped copy
  }
  else
  {
    // source string overlaps with our own memory -> we can't increase the size of our memory, as that might invalidate the source data
    PL_ASSERT_DEBUG(uiElementCount < m_Data.GetCount(), "Invalid copy of overlapping string data.");
  }

  m_Data.SetCountUninitialized(uiElementCount + 1);
  plStringUtils::Copy(&m_Data[0], uiElementCount + 1, szString);
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const plHybridStringBase& rhs)
{
  if (this == &rhs)
    return;

  m_Data = rhs.m_Data;
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(plHybridStringBase&& rhs)
{
  if (this == &rhs)
    return;

  m_Data = std::move(rhs.m_Data);
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const wchar_t* szString)
{
  plStringUtf8 sConversion(szString, m_Data.GetAllocator());
  *this = sConversion.GetData();
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const plStringView& rhs)
{
  PL_ASSERT_DEBUG(rhs.GetStartPointer() < m_Data.GetData() || rhs.GetStartPointer() >= m_Data.GetData() + m_Data.GetCount(),
    "Can't assign string a value that points to ourself!");

  m_Data.SetCountUninitialized(rhs.GetElementCount() + 1);
  plStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.GetStartPointer(), rhs.GetEndPointer());
}

template <plUInt16 Size>
plStringView plHybridStringBase<Size>::GetSubString(plUInt32 uiFirstCharacter, plUInt32 uiNumCharacters) const
{
  const char* szStart = GetData();
  if (plUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter).Failed())
    return {}; // szStart was moved too far, the result is just an empty string

  const char* szEnd = szStart;
  plUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters).IgnoreResult(); // if it fails, szEnd just points to the end of this string

  return plStringView(szStart, szEnd);
}

template <plUInt16 Size>
plStringView plHybridStringBase<Size>::GetFirst(plUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

template <plUInt16 Size>
plStringView plHybridStringBase<Size>::GetLast(plUInt32 uiNumCharacters) const
{
  const plUInt32 uiMaxCharacterCount = GetCharacterCount();
  PL_ASSERT_DEV(uiNumCharacters < uiMaxCharacterCount, "The string only contains {0} characters, cannot return the last {1} characters.",
    uiMaxCharacterCount, uiNumCharacters);
  return GetSubString(uiMaxCharacterCount - uiNumCharacters, uiNumCharacters);
}


template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString()
  : plHybridStringBase<Size>(A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(plAllocator* pAllocator)
  : plHybridStringBase<Size>(pAllocator)
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const plHybridString<Size, A>& other)
  : plHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const plHybridStringBase<Size>& other)
  : plHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(plHybridString<Size, A>&& other)
  : plHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(plHybridStringBase<Size>&& other)
  : plHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const char* rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const wchar_t* rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const plStringView& rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const plHybridString<Size, A>& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const plHybridStringBase<Size>& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(plHybridString<Size, A>&& rhs)
{
  plHybridStringBase<Size>::operator=(std::move(rhs));
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(plHybridStringBase<Size>&& rhs)
{
  plHybridStringBase<Size>::operator=(std::move(rhs));
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const char* rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const wchar_t* rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const plStringView& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

#if PL_ENABLED(PL_INTEROP_STL_STRINGS)

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const std::string_view& rhs, plAllocator* pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const std::string& rhs, plAllocator* pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const std::string_view& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    m_Data.SetCountUninitialized(((plUInt32)rhs.size() + 1));
    plStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.data(), rhs.data() + rhs.size());
  }
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const std::string& rhs)
{
  *this = std::string_view(rhs);
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const std::string_view& rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const std::string& rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const std::string_view& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PL_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const std::string& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

#endif

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
