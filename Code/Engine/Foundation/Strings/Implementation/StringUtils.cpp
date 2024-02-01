#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringView.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
#  include <Foundation/Logging/Log.h>

plAtomicInteger32 plStringUtils::g_MaxUsedStringLength;
plAtomicInteger32 plStringUtils::g_UsedStringLengths[256];

void plStringUtils::AddUsedStringLength(plUInt32 uiLength)
{
  g_MaxUsedStringLength.Max(uiLength);

  if (uiLength > 255)
    uiLength = 255;

  g_UsedStringLengths[uiLength].Increment();
}

void plStringUtils::PrintStringLengthStatistics()
{
  PL_LOG_BLOCK("String Length Statistics");

  plLog::Info("Max String Length: {0}", (plInt32)g_MaxUsedStringLength);

  plUInt32 uiCopiedStrings = 0;
  for (plUInt32 i = 0; i < 256; ++i)
    uiCopiedStrings += g_UsedStringLengths[i];

  plLog::Info("Number of String Copies: {0}", uiCopiedStrings);
  plLog::Info("");

  plUInt32 uiPercent = 0;
  plUInt32 uiStrings = 0;
  for (plUInt32 i = 0; i < 256; ++i)
  {
    if (100.0f * (uiStrings + g_UsedStringLengths[i]) / (float)uiCopiedStrings >= uiPercent)
    {
      plLog::Info("{0}%% of all Strings are shorter than {1} Elements.", plArgI(uiPercent, 3), plArgI(i + 1, 3));
      uiPercent += 10;
    }

    uiStrings += g_UsedStringLengths[i];
  }
}

#endif

// Unicode ToUpper / ToLower character conversion
//  License: $(WEB www.boost.org/LICENSE_1_0.txt, Boost License 1.0).
//  Authors: $(WEB digitalmars.com, Walter Bright), Jonathan M Davis, and Kenji Hara
//  Source: $(PHOBOSSRC std/_uni.d)
plUInt32 plStringUtils::ToUpperChar(plUInt32 uiWc)
{
  if (uiWc >= 'a' && uiWc <= 'z')
  {
    uiWc -= 'a' - 'A';
  }
  else if (uiWc >= 0x00E0)
  {
    if ((/*wc >= 0x00E0 &&*/ uiWc <= 0x00F6) || (uiWc >= 0x00F8 && uiWc <= 0x00FE))
    {
      uiWc -= 32;
    }
    else if (uiWc == 0x00FF)
      uiWc = 0x0178;
    else if ((uiWc >= 0x0100 && uiWc < 0x0138) || (uiWc > 0x0149 && uiWc < 0x0178))
    {
      if (uiWc == 0x0131)
      {
        // note:
        // this  maps the character into the ASCII range and thus changes its size in UTF-8 from
        // 2 bytes to 1 bytes, and this mapping is irreversible
        uiWc = 0x0049; // 'I'
      }
      else if (uiWc == 0x0130)
      {
        // note:
        // the character 0x130 maps to 'i' in ToLower, but usually would note get changed in ToUpper
        // however that means ToUpper(ToLower(0x130)) != ToUpper(0x130)
        // therefore, although this might "break the language", we ALWAYS convert it to the ASCII character
        // that would result if we would to ToUpper(ToLower(0x130))
        uiWc = 0x0049; // 'I'
      }
      else if (uiWc & 1)
        --uiWc;
    }
    else if ((uiWc >= 0x0139 && uiWc < 0x0149) || (uiWc > 0x0178 && uiWc < 0x017F))
    {
      if ((uiWc & 1) == 0)
        --uiWc;
    }
    else if (uiWc == 0x017F)
    {
      // note:
      // this  maps the character into the ASCII range and thus changes its size in UTF-8 from
      // 2 bytes to 1 bytes, and this mapping is irreversible
      uiWc = 0x0053; // 'S'

      // this one character means, that for case-insensitive comparisons we always need to use ToUpper
      // and NOT ToLower, as ToLower will not convert this one character, such that two strings, one with 0x017f
      // and one with 0x0053, will not compare equal
    }
    else if (uiWc >= 0x0200 && uiWc <= 0x0217)
    {
      if (uiWc & 1)
        --uiWc;
    }
    else if (uiWc >= 0x0430 && uiWc <= 0x044F)
      uiWc -= 32;
    else if ((uiWc >= 0x0451 && uiWc <= 0x045C) || (uiWc >= 0x045E && uiWc <= 0x045F))
    {
      uiWc -= 80;
    }
    else if (uiWc >= 0x0460 && uiWc <= 0x047F)
    {
      if (uiWc & 1)
        --uiWc;
    }
    else if (uiWc >= 0x0561 && uiWc < 0x0587)
      uiWc -= 48;
    else if (uiWc >= 0xFF41 && uiWc <= 0xFF5A)
      uiWc -= 32;
  }

  return uiWc;
}

// Unicode ToUpper / ToLower character conversion
//  License: $(WEB www.boost.org/LICENSE_1_0.txt, Boost License 1.0).
//  Authors: $(WEB digitalmars.com, Walter Bright), Jonathan M Davis, and Kenji Hara
//  Source: $(PHOBOSSRC std/_uni.d)
plUInt32 plStringUtils::ToLowerChar(plUInt32 uiWc)
{
  if (uiWc >= 'A' && uiWc <= 'Z')
  {
    uiWc += 'a' - 'A';
  }
  else if (uiWc >= 0x00C0)
  {
    if ((/*wc >= 0x00C0 &&*/ uiWc <= 0x00D6) || (uiWc >= 0x00D8 && uiWc <= 0x00DE))
    {
      uiWc += 32;
    }
    else if ((uiWc >= 0x0100 && uiWc < 0x0138) || (uiWc > 0x0149 && uiWc < 0x0178))
    {
      if (uiWc == 0x0130)
      {
        // note:
        // this  maps the character into the ASCII range and thus changes its size in UTF-8 from
        // 2 bytes to 1 bytes, and this mapping is irreversible
        uiWc = 0x0069; // 'i'
      }
      else if (uiWc == 0x0131)
      {
        // note:
        // the character 0x131 maps to 'I' in ToUpper, but usually would note get changed in ToLower
        // however that means ToLower(ToUpper(0x131)) != ToLower(0x131)
        // therefore, although this might "break the language", we ALWAYS convert it to the ASCII character
        // that would result if we would to ToLower(ToUpper(0x131))
        uiWc = 0x0069; // 'i'
      }
      else if ((uiWc & 1) == 0)
        ++uiWc;
    }
    else if (uiWc == 0x0178)
      uiWc = 0x00FF;
    else if ((uiWc >= 0x0139 && uiWc < 0x0149) || (uiWc > 0x0178 && uiWc < 0x017F))
    {
      if (uiWc & 1)
        ++uiWc;
    }
    else if (uiWc >= 0x0200 && uiWc <= 0x0217)
    {
      if ((uiWc & 1) == 0)
        ++uiWc;
    }
    else if ((uiWc >= 0x0401 && uiWc <= 0x040C) || (uiWc >= 0x040E && uiWc <= 0x040F))
    {
      uiWc += 80;
    }
    else if (uiWc >= 0x0410 && uiWc <= 0x042F)
      uiWc += 32;
    else if (uiWc >= 0x0460 && uiWc <= 0x047F)
    {
      if ((uiWc & 1) == 0)
        ++uiWc;
    }
    else if (uiWc >= 0x0531 && uiWc <= 0x0556)
      uiWc += 48;
    else if (uiWc >= 0x10A0 && uiWc <= 0x10C5)
      uiWc += 48;
    else if (uiWc >= 0xFF21 && uiWc <= 0xFF3A)
      uiWc += 32;
  }

  return uiWc;
}


plUInt32 plStringUtils::ToUpperString(char* pString, const char* pStringEnd)
{
  char* pWriteStart = pString;
  const char* pReadStart = pString;

  while ((pReadStart < pStringEnd) && (*pReadStart != '\0'))
  {
    const plUInt32 uiChar = plUnicodeUtils::DecodeUtf8ToUtf32(pReadStart);
    const plUInt32 uiCharUpper = plStringUtils::ToUpperChar(uiChar);
    pWriteStart = utf8::unchecked::utf32to8(&uiCharUpper, &uiCharUpper + 1, pWriteStart);
  }

  *pWriteStart = '\0';

  const plUInt32 uiNewStringLength = (plUInt32)(pWriteStart - pString);
  return uiNewStringLength;
}

plUInt32 plStringUtils::ToLowerString(char* pString, const char* pStringEnd)
{
  char* pWriteStart = pString;
  const char* pReadStart = pString;

  while ((pReadStart < pStringEnd) && (*pReadStart != '\0'))
  {
    const plUInt32 uiChar = plUnicodeUtils::DecodeUtf8ToUtf32(pReadStart);
    const plUInt32 uiCharUpper = plStringUtils::ToLowerChar(uiChar);
    pWriteStart = utf8::unchecked::utf32to8(&uiCharUpper, &uiCharUpper + 1, pWriteStart);
  }

  *pWriteStart = '\0';

  const plUInt32 uiNewStringLength = (plUInt32)(pWriteStart - pString);
  return uiNewStringLength;
}

// Macro to Handle nullptr-pointer strings
#define PL_STRINGCOMPARE_HANDLE_NULL_PTRS(szString1, szString2, ret_equal, ret_str2_larger, ret_str1_larger, szString1End, szString2End)   \
  if (szString1 == szString2) /* Handles the case that both are nullptr and that both are actually the same string */                      \
  {                                                                                                                                        \
    if ((szString1 == nullptr) || (szString1End == szString2End)) /* if both are nullptr, ignore the end pointer, otherwise the strings    \
                                                                     are equal, if both end pointers are also the same */                  \
      return (ret_equal);                                                                                                                  \
  }                                                                                                                                        \
  if (szString1 == nullptr)                                                                                                                \
  {                                                                                                                                        \
    if (szString2[0] == '\0') /* if String1 is nullptr, String2 is never nullptr, otherwise the previous IF would have returned already */ \
      return (ret_equal);                                                                                                                  \
    else                                                                                                                                   \
      return (ret_str2_larger);                                                                                                            \
  }                                                                                                                                        \
  if (szString2 == nullptr)                                                                                                                \
  {                                                                                                                                        \
    if (szString1[0] == '\0') /* if String2 is nullptr, String1 is never nullptr, otherwise the previous IF would have returned already */ \
      return (ret_equal);                                                                                                                  \
    else                                                                                                                                   \
      return (ret_str1_larger);                                                                                                            \
  }

#define ToSignedInt(c) ((plInt32)((unsigned char)c))

plInt32 plStringUtils::Compare(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  PL_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((pString1 < pString1End) && (pString2 < pString2End) && (*pString1 != '\0') && (*pString2 != '\0'))
  {
    if (*pString1 != *pString2)
      return ToSignedInt(*pString1) - ToSignedInt(*pString2);

    ++pString1;
    ++pString2;
  }

  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0;

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

plInt32 plStringUtils::CompareN(
  const char* pString1, const char* pString2, plUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  if (uiCharsToCompare == 0)
    return 0;

  PL_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((uiCharsToCompare > 0) && (pString1 < pString1End) && (pString2 < pString2End) && (*pString1 != '\0') && (*pString2 != '\0'))
  {
    if (*pString1 != *pString2)
      return ToSignedInt(*pString1) - ToSignedInt(*pString2);

    if (!plUnicodeUtils::IsUtf8ContinuationByte(*pString1))
      --uiCharsToCompare;

    ++pString1;
    ++pString2;
  }

  if (uiCharsToCompare == 0)
    return 0;

  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0; // both reached their end pointer

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

plInt32 plStringUtils::Compare_NoCase(const char* pString1, const char* pString2, const char* pString1End, const char* pString2End)
{
  PL_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((pString1 < pString1End) && (pString2 < pString2End) && (*pString1 != '\0') && (*pString2 != '\0'))
  {
    // utf8::next will already advance the iterators
    const plUInt32 uiChar1 = plUnicodeUtils::DecodeUtf8ToUtf32(pString1);
    const plUInt32 uiChar2 = plUnicodeUtils::DecodeUtf8ToUtf32(pString2);

    const plInt32 iComparison = CompareChars_NoCase(uiChar1, uiChar2);

    if (iComparison != 0)
      return iComparison;
  }


  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0; // both reached their end pointer

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    // none of them has reached their end pointer, but at least one is 0, so no need to ToUpper
    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

plInt32 plStringUtils::CompareN_NoCase(
  const char* pString1, const char* pString2, plUInt32 uiCharsToCompare, const char* pString1End, const char* pString2End)
{
  if (uiCharsToCompare == 0)
    return 0;

  PL_STRINGCOMPARE_HANDLE_NULL_PTRS(pString1, pString2, 0, -1, 1, pString1End, pString2End);

  while ((uiCharsToCompare > 0) && (pString1 < pString1End) && (pString2 < pString2End) && (*pString1 != '\0') && (*pString2 != '\0'))
  {
    // utf8::next will already advance the iterators
    const plUInt32 uiChar1 = plUnicodeUtils::DecodeUtf8ToUtf32(pString1);
    const plUInt32 uiChar2 = plUnicodeUtils::DecodeUtf8ToUtf32(pString2);

    const plInt32 iComparison = CompareChars_NoCase(uiChar1, uiChar2);

    if (iComparison != 0)
      return iComparison;

    --uiCharsToCompare;
  }

  if (uiCharsToCompare == 0)
    return 0;

  if (pString1 >= pString1End)
  {
    if (pString2 >= pString2End)
      return 0; // both reached their end pointer

    return -ToSignedInt(*pString2); // either also '\0' (end) or not 0, thus 'smaller' than pString1 (negated)
  }
  else
  {
    if (pString2 >= pString2End)
      return ToSignedInt(*pString1); // either also '\0' (end) or not 0, thus 'larger' than pString2

    // none of them has reached their end pointer, but at least one is 0, so no need to ToUpper
    return ToSignedInt(*pString1) - ToSignedInt(*pString2);
  }
}

plUInt32 plStringUtils::Copy(char* szDest, plUInt32 uiDstSize, const char* szSource, const char* pSourceEnd)
{
  PL_ASSERT_DEBUG(szDest != nullptr && uiDstSize > 0, "Invalid output buffer.");

  if (IsNullOrEmpty(szSource))
  {
    plStringUtils::AddUsedStringLength(0);
    szDest[0] = '\0';
    return 0;
  }

  plUInt32 uiSourceLen = static_cast<plUInt32>((pSourceEnd == plUnicodeUtils::GetMaxStringEnd<char>()) ? strlen(szSource) : pSourceEnd - szSource);
  plUInt32 uiBytesToCopy = plMath::Min(uiDstSize - 1, uiSourceLen);

  // simply copy all bytes
  memmove(szDest, szSource, uiBytesToCopy);

  char* szLastCharacterPos = szDest + uiBytesToCopy;

// Check if we just copied half a UTF-8 character
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  if (uiBytesToCopy > 0)
  {
    char* szUtf8StartByte = szLastCharacterPos - 1;
    while (!plUnicodeUtils::IsUtf8StartByte(*szUtf8StartByte) && szUtf8StartByte > szDest)
    {
      szUtf8StartByte--;
    }
    std::ptrdiff_t isLength = szLastCharacterPos - szUtf8StartByte;
    std::ptrdiff_t expectedLength = plUnicodeUtils::GetUtf8SequenceLength(*szUtf8StartByte);
    PL_ASSERT_DEBUG(isLength == expectedLength, "The destination buffer was too small, so a utf-8 byte sequence got cut off. This function "
                                                "is not designed to copy into buffers that are too small.");
  }
#endif


  // make sure the buffer is always terminated
  *szLastCharacterPos = '\0';

  const plUInt32 uiLength = (plUInt32)(szLastCharacterPos - szDest);

  plStringUtils::AddUsedStringLength(uiLength);
  return uiLength;
}

plUInt32 plStringUtils::CopyN(char* szDest, plUInt32 uiDstSize, const char* szSource, plUInt32 uiCharsToCopy, const char* pSourceEnd)
{
  PL_ASSERT_DEBUG(szDest != nullptr && uiDstSize > 0, "Invalid output buffer.");

  if (IsNullOrEmpty(szSource))
  {
    plStringUtils::AddUsedStringLength(0);
    szDest[0] = '\0';
    return 0;
  }

  const char* szStartPos = szDest;

  char* szLastCharacterPos = szDest;

  plInt32 iCharsCopied = -1;

  while (uiDstSize > 0)
  {
    if ((*szSource == '\0') || (szSource >= pSourceEnd))
    {
      szLastCharacterPos = szDest;
      break;
    }

    if (!plUnicodeUtils::IsUtf8ContinuationByte(*szSource))
    {
      // if this is not a continuation byte, we have copied another character into the output buffer
      ++iCharsCopied;

      // keep track of where we started writing the current character
      // because if the output buffer is not large enough, to hold the entire UTF-8 sequence, we must truncate the string
      // at the last character boundary
      szLastCharacterPos = szDest;

      // if we successfully copied enough characters, the only thing left is to terminate the string
      if (iCharsCopied == (int)uiCharsToCopy)
        break;
    }

    *szDest = *szSource;
    ++szSource;
    ++szDest;
    --uiDstSize;
  }

  // this will actually overwrite the last byte that we wrote into the output buffer
  *szLastCharacterPos = '\0';

  const plUInt32 uiLength = (plUInt32)(szLastCharacterPos - szStartPos);

  plStringUtils::AddUsedStringLength(uiLength);
  return uiLength;
}

bool plStringUtils::StartsWith(const char* szString, const char* szStartsWith, const char* pStringEnd, const char* szStartsWithEnd)
{
  if (IsNullOrEmpty(szStartsWith, szStartsWithEnd))
    return true;
  if (IsNullOrEmpty(szString, pStringEnd))
    return false;

  while ((szString < pStringEnd) && (*szString != '\0'))
  {
    // if we have reached the end of the StartsWith string, the other string DOES start with it
    if (*szStartsWith == '\0' || szStartsWith == szStartsWithEnd)
      return true;

    if (*szStartsWith != *szString)
      return false;

    ++szString;
    ++szStartsWith;
  }

  // if both are equally long, this comparison will return true
  return (*szStartsWith == '\0' || szStartsWith == szStartsWithEnd);
}

bool plStringUtils::StartsWith_NoCase(const char* szString, const char* szStartsWith, const char* pStringEnd, const char* szStartsWithEnd)
{
  if (IsNullOrEmpty(szStartsWith, szStartsWithEnd))
    return true;
  if (IsNullOrEmpty(szString, pStringEnd))
    return false;

  while ((szString < pStringEnd) && (*szString != '\0'))
  {
    // if we have reached the end of the StartsWith string, the other string DOES start with it
    if (*szStartsWith == '\0' || szStartsWith == szStartsWithEnd)
      return true;

    if (plStringUtils::CompareChars_NoCase(szStartsWith, szString) != 0)
      return false;

    plUnicodeUtils::MoveToNextUtf8(szString, pStringEnd).AssertSuccess();
    plUnicodeUtils::MoveToNextUtf8(szStartsWith, szStartsWithEnd).AssertSuccess();
  }

  // if both are equally long, this comparison will return true
  return (*szStartsWith == '\0' || szStartsWith == szStartsWithEnd);
}

bool plStringUtils::EndsWith(const char* szString, const char* szEndsWith, const char* pStringEnd, const char* szEndsWithEnd)
{
  if (IsNullOrEmpty(szEndsWith, szEndsWithEnd))
    return true;
  if (IsNullOrEmpty(szString, pStringEnd))
    return false;

  const plUInt32 uiLength1 = plStringUtils::GetStringElementCount(szString, pStringEnd);
  const plUInt32 uiLength2 = plStringUtils::GetStringElementCount(szEndsWith, szEndsWithEnd);

  if (uiLength1 < uiLength2)
    return false;

  return IsEqual(&szString[uiLength1 - uiLength2], szEndsWith, pStringEnd, szEndsWithEnd);
}

bool plStringUtils::EndsWith_NoCase(const char* szString, const char* szEndsWith, const char* pStringEnd, const char* szEndsWithEnd)
{
  if (IsNullOrEmpty(szEndsWith, szEndsWithEnd))
    return true;
  if (IsNullOrEmpty(szString, pStringEnd))
    return false;

  const plUInt32 uiLength1 = plStringUtils::GetStringElementCount(szString, pStringEnd);
  const plUInt32 uiLength2 = plStringUtils::GetStringElementCount(szEndsWith, szEndsWithEnd);

  const char* pCur1 = szString + uiLength1;   // points to \0
  const char* pCur2 = szEndsWith + uiLength2; // points to \0

  while (pCur1 > szString)
  {
    // if that string has reached its beginning, all comparisons succeeded and szString does end with szEndsWith
    if (pCur2 <= szEndsWith)
      return true;

    // move to the previous character
    plUnicodeUtils::MoveToPriorUtf8(pCur1, szString).AssertSuccess();
    plUnicodeUtils::MoveToPriorUtf8(pCur2, szEndsWith).AssertSuccess();

    if (plStringUtils::CompareChars_NoCase(pCur1, pCur2) != 0)
      return false;
  }

  // we have reached the beginning of szString
  // but we may have simultaneously reached the beginning of szEndsWith
  // so if pCur2 has reached the beginning of szEndsWith, both strings are equal, otherwise szString does not end with szEndsWith
  return (pCur2 <= szEndsWith);
}

const char* plStringUtils::FindSubString(const char* szSource, const char* szStringToFind, const char* pSourceEnd, const char* szStringToFindEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  const char* pCurPos = &szSource[0];

  while ((pCurPos < pSourceEnd) && (*pCurPos != '\0'))
  {
    if (plStringUtils::StartsWith(pCurPos, szStringToFind, pSourceEnd, szStringToFindEnd))
      return pCurPos;

    plUnicodeUtils::MoveToNextUtf8(pCurPos, pSourceEnd).AssertSuccess();
  }

  return nullptr;
}

const char* plStringUtils::FindSubString_NoCase(const char* szSource, const char* szStringToFind, const char* pSourceEnd, const char* szStringToFindEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  const char* pCurPos = &szSource[0];

  while ((pCurPos < pSourceEnd) && (*pCurPos != '\0'))
  {
    if (plStringUtils::StartsWith_NoCase(pCurPos, szStringToFind, pSourceEnd, szStringToFindEnd))
      return pCurPos;

    plUnicodeUtils::MoveToNextUtf8(pCurPos, pSourceEnd).AssertSuccess();
  }

  return nullptr;
}


const char* plStringUtils::FindLastSubString(const char* szSource, const char* szStringToFind, const char* szStartSearchAt, const char* pSourceEnd, const char* szStringToFindEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  // get the last element (actually the \0 terminator)
  if (szStartSearchAt == nullptr)
    szStartSearchAt = szSource + plStringUtils::GetStringElementCount(szSource, pSourceEnd);

  // while we haven't reached the stars .. erm, start
  while (szStartSearchAt > szSource)
  {
    plUnicodeUtils::MoveToPriorUtf8(szStartSearchAt, szSource).AssertSuccess();

    if (plStringUtils::StartsWith(szStartSearchAt, szStringToFind, pSourceEnd, szStringToFindEnd))
      return szStartSearchAt;
  }

  return nullptr;
}

const char* plStringUtils::FindLastSubString_NoCase(const char* szSource, const char* szStringToFind, const char* szStartSearchAt, const char* pSourceEnd, const char* szStringToFindEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szSource)) || (IsNullOrEmpty(szStringToFind)))
    return nullptr;

  if (szStartSearchAt == nullptr)
    szStartSearchAt = szSource + plStringUtils::GetStringElementCount(szSource, pSourceEnd);

  while (szStartSearchAt > szSource)
  {
    plUnicodeUtils::MoveToPriorUtf8(szStartSearchAt, szSource).AssertSuccess();

    if (plStringUtils::StartsWith_NoCase(szStartSearchAt, szStringToFind, pSourceEnd, szStringToFindEnd))
      return szStartSearchAt;
  }

  return nullptr;
}


const char* plStringUtils::FindWholeWord(const char* szString, const char* szSearchFor, PL_CHARACTER_FILTER isDelimiterCB, const char* pStringEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szString)) || (IsNullOrEmpty(szSearchFor)))
    return nullptr;

  const plUInt32 uiSearchedWordLength = GetStringElementCount(szSearchFor);

  const char* pPrevPos = nullptr;
  const char* pCurPos = szString;

  while ((pCurPos < pStringEnd) && (*pCurPos != '\0'))
  {
    if (StartsWith(pCurPos, szSearchFor, pStringEnd)) // yay, we found a substring, now make sure it is a 'whole word'
    {
      if (((szString == pCurPos) || // the start of the string is always a word delimiter
            (isDelimiterCB(
              plUnicodeUtils::ConvertUtf8ToUtf32(pPrevPos) /* front */))) &&                                 // make sure the character before this substring is a word delimiter
          ((pCurPos + uiSearchedWordLength >= pStringEnd) ||                                                 // the end of the string is also always a delimiter
            (isDelimiterCB(plUnicodeUtils::ConvertUtf8ToUtf32(pCurPos + uiSearchedWordLength) /* back */)))) // and the character after it, as well
        return pCurPos;
    }

    pPrevPos = pCurPos;
    plUnicodeUtils::MoveToNextUtf8(pCurPos, pStringEnd).AssertSuccess();
  }

  return nullptr;
}

const char* plStringUtils::FindWholeWord_NoCase(const char* szString, const char* szSearchFor, PL_CHARACTER_FILTER isDelimiterCB, const char* pStringEnd)
{
  // Handle nullptr-pointer strings
  if ((IsNullOrEmpty(szString)) || (IsNullOrEmpty(szSearchFor)))
    return nullptr;

  const plUInt32 uiSearchedWordLength = GetStringElementCount(szSearchFor);

  const char* pPrevPos = nullptr;
  const char* pCurPos = szString;

  while ((pCurPos < pStringEnd) && (*pCurPos != '\0'))
  {
    if (StartsWith_NoCase(pCurPos, szSearchFor, pStringEnd)) // yay, we found a substring, now make sure it is a 'whole word'
    {
      if (((szString == pCurPos) || // the start of the string is always a word delimiter
            (isDelimiterCB(
              plUnicodeUtils::ConvertUtf8ToUtf32(pPrevPos) /* front */))) &&                              // make sure the character before this substring is a word delimiter
          (isDelimiterCB(plUnicodeUtils::ConvertUtf8ToUtf32(pCurPos + uiSearchedWordLength) /* back */))) // and the character after it, as well
        return pCurPos;
    }

    pPrevPos = pCurPos;
    plUnicodeUtils::MoveToNextUtf8(pCurPos, pStringEnd).AssertSuccess();
  }

  return nullptr;
}

plResult plStringUtils::FindUIntAtTheEnd(const char* szString, plUInt32& out_uiValue, plUInt32* pStringLengthBeforeUInt /*= nullptr*/)
{
  if (plStringUtils::IsNullOrEmpty(szString))
    return PL_FAILURE;

  const char* szWork = szString;
  const char* szNumberStart = nullptr;

  while (*szWork != '\0')
  {
    char cChar = *szWork;

    if (cChar >= '0' && cChar <= '9')
    {
      if (szNumberStart == nullptr)
      {
        szNumberStart = szWork;
      }
    }
    else
    {
      szNumberStart = nullptr;
    }

    szWork++;
  }

  if (szNumberStart != nullptr)
  {
    plInt64 iResult = 0;
    if (plConversionUtils::StringToInt64(szNumberStart, iResult).Failed() || iResult < 0 || iResult > 0xFFFFFFFFu)
    {
      return PL_FAILURE;
    }

    out_uiValue = static_cast<plUInt32>(iResult);

    if (pStringLengthBeforeUInt != nullptr)
    {
      *pStringLengthBeforeUInt = static_cast<plUInt32>(szNumberStart - szString);
    }

    return PL_SUCCESS;
  }
  else
  {
    return PL_FAILURE;
  }
}

const char* plStringUtils::SkipCharacters(const char* szString, PL_CHARACTER_FILTER skipCharacterCB, bool bAlwaysSkipFirst)
{
  PL_ASSERT_DEBUG(szString != nullptr, "Invalid string");

  while (*szString != '\0')
  {
    if (!bAlwaysSkipFirst && !skipCharacterCB(plUnicodeUtils::ConvertUtf8ToUtf32(szString)))
      break;

    bAlwaysSkipFirst = false;

    plUnicodeUtils::MoveToNextUtf8(szString).AssertSuccess();
  }

  return szString;
}

const char* plStringUtils::FindWordEnd(const char* szString, PL_CHARACTER_FILTER isDelimiterCB, bool bAlwaysSkipFirst)
{
  PL_ASSERT_DEBUG(szString != nullptr, "Invalid string");

  while (*szString != '\0')
  {
    if (!bAlwaysSkipFirst && isDelimiterCB(plUnicodeUtils::ConvertUtf8ToUtf32(szString)))
      break;

    bAlwaysSkipFirst = false;
    plUnicodeUtils::MoveToNextUtf8(szString).AssertSuccess();
  }

  return szString;
}

void plStringUtils::Trim(const char*& ref_pString, const char*& ref_pStringEnd, const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  bool bTrimmed = false;
  UpdateStringEnd(ref_pString, ref_pStringEnd);
  plStringView view(ref_pString, ref_pStringEnd);
  plStringView trimFront(szTrimCharsStart);
  plStringView trimEnd(szTrimCharsEnd);

  // Trim start
  auto itStart = begin(view);
  if (!itStart.IsValid())
    return;

  do
  {
    bTrimmed = false;
    for (plUInt32 needle : trimFront)
    {
      while (itStart.GetCharacter() == needle)
      {
        ++itStart;
        bTrimmed = true;
      }
    }
  } while (bTrimmed && itStart.IsValid());
  ref_pString = itStart.GetData();
  view.SetStartPosition(ref_pString);

  // Trim end
  auto itEnd = rbegin(view);
  if (!itEnd.IsValid())
    return;

  do
  {
    bTrimmed = false;
    for (plUInt32 needle : trimEnd)
    {
      while (itEnd.GetCharacter() == needle)
      {
        ref_pStringEnd = itEnd.GetData();
        ++itEnd;
        bTrimmed = true;
      }
    }
  } while (bTrimmed && itEnd.IsValid());
}


bool plStringUtils::IsWhiteSpace(plUInt32 c)
{
  // ASCII range of useless characters (32 is actually SPACE)
  return (c >= 1 && c <= 32);
}

bool plStringUtils::IsWordDelimiter_English(plUInt32 uiChar)
{
  if ((uiChar >= 'a') && (uiChar <= 'z'))
    return false;
  if ((uiChar >= 'A') && (uiChar <= 'Z'))
    return false;
  if ((uiChar >= '0') && (uiChar <= '9'))
    return false;
  if (uiChar == '_')
    return false;
  if (uiChar == '-')
    return false;

  return true;
}

bool plStringUtils::IsIdentifierDelimiter_C_Code(plUInt32 uiChar)
{
  if ((uiChar >= 'a') && (uiChar <= 'z'))
    return false;
  if ((uiChar >= 'A') && (uiChar <= 'Z'))
    return false;
  if ((uiChar >= '0') && (uiChar <= '9'))
    return false;
  if (uiChar == '_')
    return false;

  return true;
}

bool plStringUtils::IsValidIdentifierName(const char* pString, const char* pStringEnd /*= plMaxStringEnd*/)
{
  if (IsNullOrEmpty(pString, pStringEnd))
    return false;

  plUInt32 cur = plUnicodeUtils::ConvertUtf8ToUtf32(pString);

  // digits are not allowed as the first character
  if ((cur >= '0') && (cur <= '9'))
    return false;

  while ((pString < pStringEnd) && (*pString != '\0'))
  {
    cur = plUnicodeUtils::DecodeUtf8ToUtf32(pString);

    if (plStringUtils::IsIdentifierDelimiter_C_Code(cur))
      return false;
  }

  return true;
}
