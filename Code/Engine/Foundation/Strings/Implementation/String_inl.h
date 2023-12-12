#pragma once

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  Clear();
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const plHybridStringBase& rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(plHybridStringBase&& rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  operator=(std::move(rhs));
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const char* rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const wchar_t* rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  *this = rhs;
}

template <plUInt16 Size>
plHybridStringBase<Size>::plHybridStringBase(const plStringView& rhs, plAllocatorBase* pAllocator)
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
  m_uiCharacterCount = 0;
}

template <plUInt16 Size>
PLASMA_ALWAYS_INLINE const char* plHybridStringBase<Size>::GetData() const
{
  PLASMA_ASSERT_DEBUG(!m_Data.IsEmpty(), "plHybridString has been corrupted, the array can never be empty. This can happen when you access a "
                                     "string that was previously std::move'd into another string.");

  return &m_Data[0];
}

template <plUInt16 Size>
PLASMA_ALWAYS_INLINE plUInt32 plHybridStringBase<Size>::GetElementCount() const
{
  return m_Data.GetCount() - 1;
}

template <plUInt16 Size>
PLASMA_ALWAYS_INLINE plUInt32 plHybridStringBase<Size>::GetCharacterCount() const
{
  return m_uiCharacterCount;
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const char* szString)
{
  plUInt32 uiElementCount = 0;
  plStringUtils::GetCharacterAndElementCount(szString, m_uiCharacterCount, uiElementCount);

  if (szString + uiElementCount < m_Data.GetData() || szString >= m_Data.GetData() + m_Data.GetCount())
  {
    // source string is outside our own memory, so no overlapped copy
  }
  else
  {
    // source string overlaps with our own memory -> we can't increase the size of our memory, as that might invalidate the source data
    PLASMA_ASSERT_DEBUG(uiElementCount < m_Data.GetCount(), "Invalid copy of overlapping string data.");
  }

  m_Data.SetCountUninitialized(uiElementCount + 1);
  plStringUtils::Copy(&m_Data[0], uiElementCount + 1, szString);
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(const plHybridStringBase& rhs)
{
  if (this == &rhs)
    return;

  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

template <plUInt16 Size>
void plHybridStringBase<Size>::operator=(plHybridStringBase&& rhs)
{
  if (this == &rhs)
    return;

  m_uiCharacterCount = rhs.m_uiCharacterCount;
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
  PLASMA_ASSERT_DEBUG(rhs.GetStartPointer() < m_Data.GetData() || rhs.GetStartPointer() >= m_Data.GetData() + m_Data.GetCount(),
    "Can't assign string a value that points to ourself!");

  m_Data.SetCountUninitialized(rhs.GetElementCount() + 1);
  plStringUtils::Copy(&m_Data[0], m_Data.GetCount(), rhs.GetStartPointer(), rhs.GetEndPointer());
  m_uiCharacterCount = plStringUtils::GetCharacterCount(GetData());
}

template <plUInt16 Size>
plStringView plHybridStringBase<Size>::GetSubString(plUInt32 uiFirstCharacter, plUInt32 uiNumCharacters) const
{
  PLASMA_ASSERT_DEV(uiFirstCharacter + uiNumCharacters <= m_uiCharacterCount,
    "The string only has {0} characters, cannot get a sub-string up to character {1}.", m_uiCharacterCount, uiFirstCharacter + uiNumCharacters);

  const char* szStart = GetData();
  plUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter);

  const char* szEnd = szStart;
  plUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters);

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
  PLASMA_ASSERT_DEV(uiNumCharacters < m_uiCharacterCount, "The string only contains {0} characters, cannot return the last {1} characters.",
    m_uiCharacterCount, uiNumCharacters);
  return GetSubString(m_uiCharacterCount - uiNumCharacters, uiNumCharacters);
}


template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString()
  : plHybridStringBase<Size>(A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(plAllocatorBase* pAllocator)
  : plHybridStringBase<Size>(pAllocator)
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const plHybridString<Size, A>& other)
  : plHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const plHybridStringBase<Size>& other)
  : plHybridStringBase<Size>(other, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(plHybridString<Size, A>&& other)
  : plHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(plHybridStringBase<Size>&& other)
  : plHybridStringBase<Size>(std::move(other), A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const char* rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const wchar_t* rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE plHybridString<Size, A>::plHybridString(const plStringView& rhs)
  : plHybridStringBase<Size>(rhs, A::GetAllocator())
{
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const plHybridString<Size, A>& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const plHybridStringBase<Size>& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(plHybridString<Size, A>&& rhs)
{
  plHybridStringBase<Size>::operator=(std::move(rhs));
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(plHybridStringBase<Size>&& rhs)
{
  plHybridStringBase<Size>::operator=(std::move(rhs));
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const char* rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const wchar_t* rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

template <plUInt16 Size, typename A>
PLASMA_ALWAYS_INLINE void plHybridString<Size, A>::operator=(const plStringView& rhs)
{
  plHybridStringBase<Size>::operator=(rhs);
}

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
