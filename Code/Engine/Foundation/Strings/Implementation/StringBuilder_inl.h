#pragma once

#include <Foundation/Strings/StringConversion.h>

inline plStringBuilder::plStringBuilder(plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();
}

inline plStringBuilder::plStringBuilder(const plStringBuilder& rhs)
  : m_Data(rhs.GetAllocator())
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

inline plStringBuilder::plStringBuilder(plStringBuilder&& rhs) noexcept
  : m_Data(rhs.GetAllocator())
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = std::move(rhs);
}

inline plStringBuilder::plStringBuilder(const char* szUTF8, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = szUTF8;
}

inline plStringBuilder::plStringBuilder(const wchar_t* pWChar, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = pWChar;
}

inline plStringBuilder::plStringBuilder(plStringView rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

inline plStringBuilder::plStringBuilder(const std::string_view& rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

inline plStringBuilder::plStringBuilder(const std::string& rhs, plAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

PLASMA_ALWAYS_INLINE plAllocatorBase* plStringBuilder::GetAllocator() const
{
  return m_Data.GetAllocator();
}

PLASMA_ALWAYS_INLINE void plStringBuilder::operator=(const char* szUTF8)
{
  Set(szUTF8);
}

PLASMA_FORCE_INLINE void plStringBuilder::operator=(const wchar_t* pWChar)
{
  // fine to do this, szWChar can never come from the stringbuilder's own data array
  Clear();
  Append(pWChar);
}

PLASMA_ALWAYS_INLINE void plStringBuilder::operator=(const plStringBuilder& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

PLASMA_ALWAYS_INLINE void plStringBuilder::operator=(plStringBuilder&& rhs) noexcept
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = std::move(rhs.m_Data);
}

PLASMA_ALWAYS_INLINE plUInt32 plStringBuilder::GetElementCount() const
{
  return m_Data.GetCount() - 1; // exclude the '\0' terminator
}

PLASMA_ALWAYS_INLINE plUInt32 plStringBuilder::GetCharacterCount() const
{
  return m_uiCharacterCount;
}

PLASMA_FORCE_INLINE void plStringBuilder::Clear()
{
  m_uiCharacterCount = 0;
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

inline void plStringBuilder::Append(plUInt32 uiChar)
{
  char szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar = &szChar[0];

  plUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  plUInt32 uiCharLen = (plUInt32)(pChar - szChar);
  plUInt32 uiOldCount = m_Data.GetCount();
  m_Data.SetCountUninitialized(uiOldCount + uiCharLen);
  uiOldCount--;
  for (plUInt32 i = 0; i < uiCharLen; i++)
  {
    m_Data[uiOldCount + i] = szChar[i];
  }
  m_Data[uiOldCount + uiCharLen] = '\0';
  ++m_uiCharacterCount;
}

inline void plStringBuilder::Prepend(plUInt32 uiChar)
{
  char szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar = &szChar[0];

  plUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  Prepend(szChar);
}

inline void plStringBuilder::Append(
  const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  plStringUtf8 s1(pData1, m_Data.GetAllocator());
  plStringUtf8 s2(pData2, m_Data.GetAllocator());
  plStringUtf8 s3(pData3, m_Data.GetAllocator());
  plStringUtf8 s4(pData4, m_Data.GetAllocator());
  plStringUtf8 s5(pData5, m_Data.GetAllocator());
  plStringUtf8 s6(pData6, m_Data.GetAllocator());

  Append(s1.GetView(), s2.GetView(), s3.GetView(), s4.GetView(), s5.GetView(), s6.GetView());
}

inline void plStringBuilder::Prepend(
  const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  plStringUtf8 s1(pData1, m_Data.GetAllocator());
  plStringUtf8 s2(pData2, m_Data.GetAllocator());
  plStringUtf8 s3(pData3, m_Data.GetAllocator());
  plStringUtf8 s4(pData4, m_Data.GetAllocator());
  plStringUtf8 s5(pData5, m_Data.GetAllocator());
  plStringUtf8 s6(pData6, m_Data.GetAllocator());

  Prepend(s1.GetView(), s2.GetView(), s3.GetView(), s4.GetView(), s5.GetView(), s6.GetView());
}

PLASMA_ALWAYS_INLINE const char* plStringBuilder::GetData() const
{
  PLASMA_ASSERT_DEBUG(!m_Data.IsEmpty(), "plStringBuilder has been corrupted, the array can never be empty.");

  return &m_Data[0];
}

inline void plStringBuilder::AppendTerminator()
{
  // make sure the string terminates with a zero.
  if (m_Data.IsEmpty() || (m_Data.PeekBack() != '\0'))
    m_Data.PushBack('\0');
}

inline void plStringBuilder::ToUpper()
{
  const plUInt32 uiNewStringLength = plStringUtils::ToUpperString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void plStringBuilder::ToLower()
{
  const plUInt32 uiNewStringLength = plStringUtils::ToLowerString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void plStringBuilder::ChangeCharacter(iterator& ref_it, plUInt32 uiCharacter)
{
  PLASMA_ASSERT_DEV(ref_it.IsValid(), "The given character iterator does not point to a valid character.");
  PLASMA_ASSERT_DEV(ref_it.GetData() >= GetData() && ref_it.GetData() < GetData() + GetElementCount(),
    "The given character iterator does not point into this string. It was either created from another string, or this string "
    "has been reallocated in the mean time.");

  // this is only an optimization for pure ASCII strings
  // without it, the code below would still work
  if (plUnicodeUtils::IsASCII(*ref_it) && plUnicodeUtils::IsASCII(uiCharacter))
  {
    char* pPos = const_cast<char*>(ref_it.GetData()); // yes, I know...
    *pPos = uiCharacter & 0xFF;
    return;
  }

  ChangeCharacterNonASCII(ref_it, uiCharacter);
}

PLASMA_ALWAYS_INLINE bool plStringBuilder::IsPureASCII() const
{
  return m_uiCharacterCount + 1 == m_Data.GetCount();
}

PLASMA_ALWAYS_INLINE void plStringBuilder::Reserve(plUInt32 uiNumElements)
{
  m_Data.Reserve(uiNumElements);
}

PLASMA_ALWAYS_INLINE void plStringBuilder::Insert(const char* szInsertAtPos, plStringView sTextToInsert)
{
  ReplaceSubString(szInsertAtPos, szInsertAtPos, sTextToInsert);
}

PLASMA_ALWAYS_INLINE void plStringBuilder::Remove(const char* szRemoveFromPos, const char* szRemoveToPos)
{
  ReplaceSubString(szRemoveFromPos, szRemoveToPos, plStringView());
}

template <typename Container>
bool plUnicodeUtils::RepairNonUtf8Text(const char* pStartData, const char* pEndData, Container& out_result)
{
  if (plUnicodeUtils::IsValidUtf8(pStartData, pEndData))
  {
    out_result = plStringView(pStartData, pEndData);
    return false;
  }

  out_result.Clear();

  plHybridArray<char, 1024> fixedText;
  plUnicodeUtils::UtfInserter<char, decltype(fixedText)> inserter(&fixedText);

  while (pStartData < pEndData)
  {
    const plUInt32 uiChar = plUnicodeUtils::DecodeUtf8ToUtf32(pStartData);
    plUnicodeUtils::EncodeUtf32ToUtf8(uiChar, inserter);
  }

  PLASMA_ASSERT_DEV(plUnicodeUtils::IsValidUtf8(fixedText.GetData(), fixedText.GetData() + fixedText.GetCount()), "Repaired text is still not a valid Utf8 string.");

  out_result = plStringView(fixedText.GetData(), fixedText.GetCount());
  return true;
}

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
