#pragma once

PL_ALWAYS_INLINE plInt32 plStringUtils::CompareChars(plUInt32 uiCharacter1, plUInt32 uiCharacter2)
{
  return (plInt32)uiCharacter1 - (plInt32)uiCharacter2;
}

inline plInt32 plStringUtils::CompareChars_NoCase(plUInt32 uiCharacter1, plUInt32 uiCharacter2)
{
  return (plInt32)ToUpperChar(uiCharacter1) - (plInt32)ToUpperChar(uiCharacter2);
}

inline plInt32 plStringUtils::CompareChars(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars(plUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), plUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

inline plInt32 plStringUtils::CompareChars_NoCase(const char* szUtf8Char1, const char* szUtf8Char2)
{
  return CompareChars_NoCase(plUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char1), plUnicodeUtils::ConvertUtf8ToUtf32(szUtf8Char2));
}

template <typename T>
PL_ALWAYS_INLINE constexpr bool plStringUtils::IsNullOrEmpty(const T* pString)
{
  return (pString == nullptr) || (pString[0] == '\0');
}

template <typename T>
PL_ALWAYS_INLINE bool plStringUtils::IsNullOrEmpty(const T* pString, const T* pStringEnd)
{
  return (pString == nullptr) || (pString[0] == '\0') || pString == pStringEnd;
}

template <typename T>
PL_ALWAYS_INLINE void plStringUtils::UpdateStringEnd(const T* pStringStart, const T*& ref_pStringEnd)
{
  if (ref_pStringEnd != plUnicodeUtils::GetMaxStringEnd<T>())
    return;

  ref_pStringEnd = pStringStart + GetStringElementCount(pStringStart, plUnicodeUtils::GetMaxStringEnd<T>());
}

template <typename T>
constexpr plUInt32 plStringUtils::GetStringElementCount(const T* pString)
{
  if (IsNullOrEmpty(pString))
    return 0;

  plUInt32 uiCount = 0;
  while ((*pString != '\0'))
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

template <typename T>
plUInt32 plStringUtils::GetStringElementCount(const T* pString, const T* pStringEnd)
{
  if (IsNullOrEmpty(pString))
    return 0;

  if (pStringEnd != plUnicodeUtils::GetMaxStringEnd<T>())
    return (plUInt32)(pStringEnd - pString);

  plUInt32 uiCount = 0;
  while ((*pString != '\0') && (pString < pStringEnd))
  {
    ++pString;
    ++uiCount;
  }

  return uiCount;
}

inline plUInt32 plStringUtils::GetCharacterCount(const char* szUtf8, const char* pStringEnd)
{
  if (IsNullOrEmpty(szUtf8))
    return 0;

  plUInt32 uiCharacters = 0;

  while ((*szUtf8 != '\0') && (szUtf8 < pStringEnd))
  {
    // skip all the Utf8 continuation bytes
    if (!plUnicodeUtils::IsUtf8ContinuationByte(*szUtf8))
      ++uiCharacters;

    ++szUtf8;
  }

  return uiCharacters;
}

inline void plStringUtils::GetCharacterAndElementCount(const char* szUtf8, plUInt32& ref_uiCharacterCount, plUInt32& ref_uiElementCount, const char* pStringEnd)
{
  ref_uiCharacterCount = 0;
  ref_uiElementCount = 0;

  if (IsNullOrEmpty(szUtf8))
    return;

  while (szUtf8 < pStringEnd)
  {
    char uiByte = *szUtf8;
    if (uiByte == '\0')
    {
      break;
    }

    // skip all the Utf8 continuation bytes
    if (!plUnicodeUtils::IsUtf8ContinuationByte(uiByte))
      ++ref_uiCharacterCount;

    ++szUtf8;
    ++ref_uiElementCount;
  }
}

PL_ALWAYS_INLINE bool plStringUtils::IsEqual(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return plStringUtils::Compare(pString1, pString2, pString1End, pString2End) == 0;
}

PL_ALWAYS_INLINE bool plStringUtils::IsEqualN(
  const char* pString1, const char* pString2, plUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return plStringUtils::CompareN(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

PL_ALWAYS_INLINE bool plStringUtils::IsEqual_NoCase(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  return plStringUtils::Compare_NoCase(pString1, pString2, pString1End, pString2End) == 0;
}

PL_ALWAYS_INLINE bool plStringUtils::IsEqualN_NoCase(
  const char* pString1, const char* pString2, plUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  return plStringUtils::CompareN_NoCase(pString1, pString2, uiCharsToCompare, pString1End, pString2End) == 0;
}

PL_ALWAYS_INLINE bool plStringUtils::IsDecimalDigit(plUInt32 uiChar)
{
  return (uiChar >= '0' && uiChar <= '9');
}

PL_ALWAYS_INLINE bool plStringUtils::IsHexDigit(plUInt32 uiChar)
{
  return IsDecimalDigit(uiChar) || (uiChar >= 'A' && uiChar <= 'F') || (uiChar >= 'a' && uiChar <= 'f');
}
