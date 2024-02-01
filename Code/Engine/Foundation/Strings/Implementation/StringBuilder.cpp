#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>

#include <stdarg.h>

plStringBuilder::plStringBuilder(plStringView sData1, plStringView sData2, plStringView sData3, plStringView sData4, plStringView sData5, plStringView sData6)
{
  AppendTerminator();

  Append(sData1, sData2, sData3, sData4, sData5, sData6);
}

void plStringBuilder::Set(plStringView sData1, plStringView sData2, plStringView sData3, plStringView sData4, plStringView sData5, plStringView sData6)
{
  Clear();
  Append(sData1, sData2, sData3, sData4, sData5, sData6);
}

void plStringBuilder::SetSubString_FromTo(const char* pStart, const char* pEnd)
{
  PL_ASSERT_DEBUG(plUnicodeUtils::IsValidUtf8(pStart, pEnd), "Invalid substring, the start does not point to a valid Utf-8 character");

  plStringView view(pStart, pEnd);
  *this = view;
}

void plStringBuilder::SetSubString_ElementCount(const char* pStart, plUInt32 uiElementCount)
{
  PL_ASSERT_DEBUG(
    plUnicodeUtils::IsValidUtf8(pStart, pStart + uiElementCount), "Invalid substring, the start does not point to a valid Utf-8 character");

  plStringView view(pStart, pStart + uiElementCount);
  *this = view;
}

void plStringBuilder::SetSubString_CharacterCount(const char* pStart, plUInt32 uiCharacterCount)
{
  const char* pEnd = pStart;
  plUnicodeUtils::MoveToNextUtf8(pEnd, uiCharacterCount).IgnoreResult(); // fine to fail, will just copy as much as possible

  plStringView view(pStart, pEnd);
  *this = view;
}

void plStringBuilder::Append(plStringView sData1, plStringView sData2, plStringView sData3, plStringView sData4, plStringView sData5, plStringView sData6)
{
  // it is not possible to find out how many parameters were passed to a vararg function
  // with a fixed size of parameters we do not need to have a parameter that tells us how many strings will come

  const plUInt32 uiMaxParams = 6;

  const plStringView pStrings[uiMaxParams] = {sData1, sData2, sData3, sData4, sData5, sData6};
  plUInt32 uiStrLen[uiMaxParams] = {0};

  plUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (plUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    PL_ASSERT_DEBUG(pStrings[i].GetStartPointer() < m_Data.GetData() || pStrings[i].GetStartPointer() >= m_Data.GetData() + m_Data.GetCapacity(),
      "Parameter {0} comes from the string builders own storage. This type assignment is not allowed.", i);

    uiStrLen[i] = pStrings[i].GetElementCount();
    uiMoreBytes += uiStrLen[i];

    PL_ASSERT_DEBUG(plUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  plUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  PL_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // and then append all the strings
  for (plUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    plStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen[i] + 1, pStrings[i].GetStartPointer(), pStrings[i].GetStartPointer() + uiStrLen[i]);

    uiPrevCount += uiStrLen[i];
  }
}

void plStringBuilder::Prepend(plStringView sData1, plStringView sData2, plStringView sData3, plStringView sData4, plStringView sData5, plStringView sData6)
{
  // it is not possible to find out how many parameters were passed to a vararg function
  // with a fixed size of parameters we do not need to have a parameter that tells us how many strings will come

  const plUInt32 uiMaxParams = 6;

  const plStringView pStrings[uiMaxParams] = {sData1, sData2, sData3, sData4, sData5, sData6};
  plUInt32 uiStrLen[uiMaxParams] = {0};

  plUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (plUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    uiStrLen[i] = pStrings[i].GetElementCount();
    uiMoreBytes += uiStrLen[i];

    PL_ASSERT_DEBUG(plUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  plUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  PL_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // move the previous string data at the end
  plMemoryUtils::CopyOverlapped(&m_Data[0] + uiMoreBytes, GetData(), uiPrevCount);

  plUInt32 uiWritePos = 0;

  // and then prepend all the strings
  for (plUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    plMemoryUtils::Copy(&m_Data[uiWritePos], pStrings[i].GetStartPointer(), uiStrLen[i]);

    uiWritePos += uiStrLen[i];
  }
}

void plStringBuilder::SetPrintfArgs(const char* szUtf8Format, va_list szArgs0)
{
  va_list args;
  va_copy(args, szArgs0);

  Clear();

  const plUInt32 TempBuffer = 4096;

  char szTemp[TempBuffer];
  const plInt32 iCount = plStringUtils::vsnprintf(szTemp, TempBuffer - 1, szUtf8Format, args);

  PL_ASSERT_DEV(iCount != -1, "There was an error while formatting the string. Probably and unescaped usage of the %% sign.");

  if (iCount == -1)
  {
    va_end(args);
    return;
  }

  if (iCount > TempBuffer - 1)
  {
    plDynamicArray<char> Temp;
    Temp.SetCountUninitialized(iCount + 1);

    plStringUtils::vsnprintf(&Temp[0], iCount + 1, szUtf8Format, args);

    Append(&Temp[0]);
  }
  else
  {
    Append(&szTemp[0]);
  }

  va_end(args);
}

void plStringBuilder::ChangeCharacterNonASCII(iterator& it, plUInt32 uiCharacter)
{
  char* pPos = const_cast<char*>(it.GetData()); // yes, I know...

  const plUInt32 uiOldCharLength = plUnicodeUtils::GetUtf8SequenceLength(*pPos);
  const plUInt32 uiNewCharLength = plUnicodeUtils::GetSizeForCharacterInUtf8(uiCharacter);

  // if the old character and the new one are encoded with the same length, we can replace the character in-place
  if (uiNewCharLength == uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    plUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // if the encoding length is identical, this will also handle all ASCII strings
    // if the string was pure ASCII before, this won't change, so no need to update that state
    return;
  }

  // in this case we can still update the string without reallocation, but the tail of the string has to be moved forwards
  if (uiNewCharLength < uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    plUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // pPos will be changed (moved forwards) to the next character position

    // how much has changed
    const plUInt32 uiDifference = uiOldCharLength - uiNewCharLength;
    const plUInt32 uiTrailStringBytes = (plUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1); // ???

    // move the trailing characters forwards
    plMemoryUtils::CopyOverlapped(pPos, pPos + uiDifference, uiTrailStringBytes);

    // update the data array
    m_Data.PopBack(uiDifference);

    // 'It' references this already, no need to change anything.
  }
  else
  {
    // in this case we insert a character that is longer int Utf8 encoding than the character that already exists there *sigh*
    // so we must first move the trailing string backwards to make room, then we can write the new char in there

    // how much has changed
    const plUInt32 uiDifference = uiNewCharLength - uiOldCharLength;
    const plUInt32 uiTrailStringBytes = (plUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1);
    auto iCurrentPos = (it.GetData() - GetData());
    // resize the array
    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // these might have changed (array realloc)
    pPos = &m_Data[0] + iCurrentPos;
    it.SetCurrentPosition(pPos);

    // move the trailing string backwards
    plMemoryUtils::CopyOverlapped(pPos + uiNewCharLength, pPos + uiOldCharLength, uiTrailStringBytes);

    // just overwrite all characters at the given position with the new Utf8 string
    plUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);
  }
}

void plStringBuilder::Shrink(plUInt32 uiShrinkCharsFront, plUInt32 uiShrinkCharsBack)
{
  if (uiShrinkCharsBack > 0)
  {
    const char* szEnd = GetData() + GetElementCount();
    const char* szNewEnd = szEnd;
    if (plUnicodeUtils::MoveToPriorUtf8(szNewEnd, GetData(), uiShrinkCharsBack).Failed())
    {
      Clear();
      return;
    }

    const plUInt32 uiLessBytes = (plUInt32)(szEnd - szNewEnd);

    m_Data.PopBack(uiLessBytes + 1);
    AppendTerminator();
  }

  const char* szNewStart = &m_Data[0];
  if (plUnicodeUtils::MoveToNextUtf8(szNewStart, uiShrinkCharsFront).Failed())
  {
    Clear();
    return;
  }

  if (szNewStart > &m_Data[0])
  {
    const plUInt32 uiLessBytes = (plUInt32)(szNewStart - &m_Data[0]);

    plMemoryUtils::CopyOverlapped(&m_Data[0], szNewStart, m_Data.GetCount() - uiLessBytes);
    m_Data.PopBack(uiLessBytes);
  }
}

void plStringBuilder::ReplaceSubString(const char* szStartPos, const char* szEndPos, plStringView sReplaceWith)
{
  PL_ASSERT_DEV(plMath::IsInRange(szStartPos, GetData(), GetData() + m_Data.GetCount()), "szStartPos is not inside this string.");
  PL_ASSERT_DEV(plMath::IsInRange(szEndPos, GetData(), GetData() + m_Data.GetCount()), "szEndPos is not inside this string.");
  PL_ASSERT_DEV(szStartPos <= szEndPos, "plStartPos must be before plEndPos");

  const plUInt32 uiWordBytes = sReplaceWith.GetElementCount();

  const plUInt32 uiSubStringBytes = (plUInt32)(szEndPos - szStartPos);

  char* szWritePos = const_cast<char*>(szStartPos); // szStartPos points into our own data anyway
  const char* szReadPos = sReplaceWith.GetStartPointer();

  // most simple case, just replace characters
  if (uiSubStringBytes == uiWordBytes)
  {
    while (szWritePos < szEndPos)
    {
      *szWritePos = *szReadPos;
      ++szWritePos;
      ++szReadPos;
    }

    return;
  }

  // the replacement is shorter than the existing stuff -> move characters to the left, no reallocation needed
  if (uiWordBytes < uiSubStringBytes)
  {
    // first copy the replacement to the correct position
    plMemoryUtils::Copy(szWritePos, sReplaceWith.GetStartPointer(), uiWordBytes);

    const plUInt32 uiDifference = uiSubStringBytes - uiWordBytes;

    const char* szStringEnd = GetData() + m_Data.GetCount();

    // now move all the characters from behind the replaced string to the correct position
    plMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    m_Data.PopBack(uiDifference);

    return;
  }

  // else the replacement is longer than the existing word
  {
    const plUInt32 uiDifference = uiWordBytes - uiSubStringBytes;
    const plUInt64 uiRelativeWritePosition = szWritePos - GetData();
    const plUInt64 uiDataByteCountBefore = m_Data.GetCount();

    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // all pointer are now possibly invalid since the data may be reallocated!
    szWritePos = const_cast<char*>(GetData()) + uiRelativeWritePosition;
    const char* szStringEnd = GetData() + uiDataByteCountBefore;

    // first move the characters to the proper position from back to front
    plMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    // now copy the replacement to the correct position
    plMemoryUtils::Copy(szWritePos, sReplaceWith.GetStartPointer(), uiWordBytes);
  }
}

const char* plStringBuilder::ReplaceFirst(plStringView sSearchFor, plStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    PL_ASSERT_DEV(plMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = plStringUtils::FindSubString(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const plUInt32 uiOffset = (plUInt32)(szFoundAt - GetData());

  const plUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* plStringBuilder::ReplaceLast(plStringView sSearchFor, plStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    PL_ASSERT_DEV(plMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = plStringUtils::FindLastSubString(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const plUInt32 uiOffset = (plUInt32)(szFoundAt - GetData());

  const plUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

plUInt32 plStringBuilder::ReplaceAll(plStringView sSearchFor, plStringView sReplacement)
{
  const plUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const plUInt32 uiWordBytes = sReplacement.GetElementCount();

  plUInt32 uiReplacements = 0;
  plUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = plStringUtils::FindSubString(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<plUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}


const char* plStringBuilder::ReplaceFirst_NoCase(plStringView sSearchFor, plStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    PL_ASSERT_DEV(plMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = plStringUtils::FindSubString_NoCase(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const plUInt32 uiOffset = (plUInt32)(szFoundAt - GetData());

  const plUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* plStringBuilder::ReplaceLast_NoCase(plStringView sSearchFor, plStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    PL_ASSERT_DEV(plMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = plStringUtils::FindLastSubString_NoCase(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const plUInt32 uiOffset = (plUInt32)(szFoundAt - GetData());

  const plUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

plUInt32 plStringBuilder::ReplaceAll_NoCase(plStringView sSearchFor, plStringView sReplacement)
{
  const plUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const plUInt32 uiWordBytes = sReplacement.GetElementCount();

  plUInt32 uiReplacements = 0;
  plUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = plStringUtils::FindSubString_NoCase(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<plUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}

const char* plStringBuilder::ReplaceWholeWord(const char* szSearchFor, plStringView sReplaceWith, plStringUtils::PL_CHARACTER_FILTER isDelimiterCB)
{
  const char* szPos = FindWholeWord(szSearchFor, isDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const plUInt32 uiOffset = static_cast<plUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + plStringUtils::GetStringElementCount(szSearchFor), sReplaceWith);
  return GetData() + uiOffset;
}

const char* plStringBuilder::ReplaceWholeWord_NoCase(const char* szSearchFor, plStringView sReplaceWith, plStringUtils::PL_CHARACTER_FILTER isDelimiterCB)
{
  const char* szPos = FindWholeWord_NoCase(szSearchFor, isDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const plUInt32 uiOffset = static_cast<plUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + plStringUtils::GetStringElementCount(szSearchFor), sReplaceWith);
  return GetData() + uiOffset;
}


plUInt32 plStringBuilder::ReplaceWholeWordAll(const char* szSearchFor, plStringView sReplaceWith, plStringUtils::PL_CHARACTER_FILTER isDelimiterCB)
{
  const plUInt32 uiSearchBytes = plStringUtils::GetStringElementCount(szSearchFor);
  const plUInt32 uiWordBytes = plStringUtils::GetStringElementCount(sReplaceWith.GetStartPointer(), sReplaceWith.GetEndPointer());

  plUInt32 uiReplacements = 0;
  plUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = plStringUtils::FindWholeWord(GetData() + uiOffset, szSearchFor, isDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<plUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

plUInt32 plStringBuilder::ReplaceWholeWordAll_NoCase(const char* szSearchFor, plStringView sReplaceWith, plStringUtils::PL_CHARACTER_FILTER isDelimiterCB)
{
  const plUInt32 uiSearchBytes = plStringUtils::GetStringElementCount(szSearchFor);
  const plUInt32 uiWordBytes = plStringUtils::GetStringElementCount(sReplaceWith.GetStartPointer(), sReplaceWith.GetEndPointer());

  plUInt32 uiReplacements = 0;
  plUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = plStringUtils::FindWholeWord_NoCase(GetData() + uiOffset, szSearchFor, isDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<plUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

void plStringBuilder::operator=(plStringView rhs)
{
  plUInt32 uiBytes = rhs.GetElementCount();

  // if we need more room, allocate up front (rhs cannot use our own data in this case)
  if (uiBytes + 1 > m_Data.GetCount())
    m_Data.SetCountUninitialized(uiBytes + 1);

  // the data might actually come from our very own string, so we 'move' the memory in there, just to be safe
  // if it comes from our own array, the data will always be a sub-set -> smaller than this array
  // in this case we defer the SetCount till later, to ensure that the data is not corrupted (destructed) before we copy it
  // however, when the new data is larger than the old, it cannot be from our own data, so we can (and must) reallocate before copying
  plMemoryUtils::CopyOverlapped(&m_Data[0], rhs.GetStartPointer(), uiBytes);

  m_Data.SetCountUninitialized(uiBytes + 1);
  m_Data[uiBytes] = '\0';
}

enum PathUpState
{
  NotStarted,
  OneDot,
  TwoDots,
  FoundDotSlash,
  FoundDotDotSlash,
  Invalid,
};

void plStringBuilder::MakeCleanPath()
{
  if (IsEmpty())
    return;

  Trim(" \t\r\n");

  RemoveDoubleSlashesInPath();

  // remove Windows specific DOS device path indicators from the start
  TrimWordStart("//?/");
  TrimWordStart("//./");

  const char* const szEndPos = &m_Data[m_Data.GetCount() - 1];
  const char* szCurReadPos = &m_Data[0];
  char* const szCurWritePos = &m_Data[0];
  int writeOffset = 0;

  plInt32 iLevelsDown = 0;
  PathUpState FoundPathUp = NotStarted;

  while (szCurReadPos < szEndPos)
  {
    char CurChar = *szCurReadPos;

    if (CurChar == '.')
    {
      if (FoundPathUp == NotStarted)
        FoundPathUp = OneDot;
      else if (FoundPathUp == OneDot)
        FoundPathUp = TwoDots;
      else
        FoundPathUp = Invalid;
    }
    else if (plPathUtils::IsPathSeparator(CurChar))
    {
      CurChar = '/';

      if (FoundPathUp == OneDot)
      {
        FoundPathUp = FoundDotSlash;
      }
      else if (FoundPathUp == TwoDots)
      {
        FoundPathUp = FoundDotDotSlash;
      }
      else
      {
        ++iLevelsDown;
        FoundPathUp = NotStarted;
      }
    }
    else
      FoundPathUp = NotStarted;

    if (FoundPathUp == FoundDotDotSlash)
    {
      if (iLevelsDown > 0)
      {
        --iLevelsDown;
        PL_ASSERT_DEBUG(writeOffset >= 3, "invalid write offset");
        writeOffset -= 3; // go back, skip two dots, one slash

        while ((writeOffset > 0) && (szCurWritePos[writeOffset - 1] != '/'))
        {
          PL_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
          --writeOffset;
        }
      }
      else
      {
        szCurWritePos[writeOffset] = '/';
        ++writeOffset;
      }

      FoundPathUp = NotStarted;
    }
    else if (FoundPathUp == FoundDotSlash)
    {
      PL_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
      writeOffset -= 1; // go back to where we wrote the dot

      FoundPathUp = NotStarted;
    }
    else
    {
      szCurWritePos[writeOffset] = CurChar;
      ++writeOffset;
    }

    ++szCurReadPos;
  }

  const plUInt32 uiPrevByteCount = m_Data.GetCount();
  const plUInt32 uiNewByteCount = (plUInt32)(writeOffset) + 1;

  PL_IGNORE_UNUSED(uiPrevByteCount);
  PL_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
    uiPrevByteCount, uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  szCurWritePos[writeOffset] = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}

void plStringBuilder::PathParentDirectory(plUInt32 uiLevelsUp)
{
  PL_ASSERT_DEV(uiLevelsUp > 0, "We have to do something!");

  for (plUInt32 i = 0; i < uiLevelsUp; ++i)
    AppendPath("../");

  MakeCleanPath();
}

void plStringBuilder::AppendPath(plStringView sPath1, plStringView sPath2, plStringView sPath3, plStringView sPath4)
{
  const plStringView sPaths[4] = {sPath1, sPath2, sPath3, sPath4};

  for (plUInt32 i = 0; i < 4; ++i)
  {
    plStringView sThisPath = sPaths[i];

    if (!sThisPath.IsEmpty())
    {
      if ((IsEmpty() && plPathUtils::IsAbsolutePath(sPaths[i])))
      {
        // this is for Linux systems where absolute paths start with a slash, wouldn't want to remove that
      }
      else
      {
        // prevent creating multiple path separators through concatenation
        while (plPathUtils::IsPathSeparator(*sThisPath.GetStartPointer()))
          sThisPath.ChopAwayFirstCharacterAscii();
      }

      if (IsEmpty() || plPathUtils::IsPathSeparator(GetIteratorBack().GetCharacter()))
        Append(sThisPath);
      else
        Append("/", sThisPath);
    }
  }
}

void plStringBuilder::AppendWithSeparator(plStringView sOptional, plStringView sText1, plStringView sText2 /*= plStringView()*/,
  plStringView sText3 /*= plStringView()*/, plStringView sText4 /*= plStringView()*/, plStringView sText5 /*= plStringView()*/,
  plStringView sText6 /*= plStringView()*/)
{
  // if this string already ends with the optional string, reset it to be empty
  if (IsEmpty() || plStringUtils::EndsWith(GetData(), sOptional.GetStartPointer(), GetData() + GetElementCount(), sOptional.GetEndPointer()))
  {
    sOptional = plStringView();
  }

  const plUInt32 uiMaxParams = 7;

  const plStringView pStrings[uiMaxParams] = {sOptional, sText1, sText2, sText3, sText4, sText5, sText6};
  plUInt32 uiStrLen[uiMaxParams] = {0};
  plUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (plUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    PL_ASSERT_DEBUG(pStrings[i].GetStartPointer() < m_Data.GetData() || pStrings[i].GetStartPointer() >= m_Data.GetData() + m_Data.GetCapacity(),
      "Parameter {0} comes from the string builders own storage. This type assignment is not allowed.", i);

    uiStrLen[i] = pStrings[i].GetElementCount();
    uiMoreBytes += uiStrLen[i];

    PL_ASSERT_DEV(plUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  if (uiMoreBytes == uiStrLen[0])
  {
    // if all other strings (than the separator) are empty, don't append anything
    return;
  }

  plUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  PL_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // and then append all the strings
  for (plUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    plStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen[i] + 1, pStrings[i].GetStartPointer(), pStrings[i].GetStartPointer() + uiStrLen[i]);

    uiPrevCount += uiStrLen[i];
  }
}

void plStringBuilder::ChangeFileName(plStringView sNewFileName)
{
  plStringView it = plPathUtils::GetFileName(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileName);
}

void plStringBuilder::ChangeFileNameAndExtension(plStringView sNewFileNameWithExtension)
{
  plStringView it = plPathUtils::GetFileNameAndExtension(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileNameWithExtension);
}

void plStringBuilder::ChangeFileExtension(plStringView sNewExtension)
{
  while (sNewExtension.StartsWith("."))
  {
    sNewExtension.ChopAwayFirstCharacterAscii();
  }

  const plStringView it = plPathUtils::GetFileExtension(GetView());

  if (it.IsEmpty() && !EndsWith("."))
    Append(".", sNewExtension);
  else
    ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewExtension);
}

void plStringBuilder::RemoveFileExtension()
{
  if (HasAnyExtension())
  {
    ChangeFileExtension("");
    Shrink(0, 1); // remove the dot
  }
}

plResult plStringBuilder::MakeRelativeTo(plStringView sAbsolutePathToMakeThisRelativeTo)
{
  plStringBuilder sAbsBase = sAbsolutePathToMakeThisRelativeTo;
  sAbsBase.MakeCleanPath();
  plStringBuilder sAbsThis = *this;
  sAbsThis.MakeCleanPath();

  if (sAbsBase.IsEqual_NoCase(sAbsThis.GetData()))
  {
    Clear();
    return PL_SUCCESS;
  }

  if (!sAbsBase.EndsWith("/"))
    sAbsBase.Append("/");

  if (!sAbsThis.EndsWith("/"))
  {
    sAbsThis.Append("/");

    if (sAbsBase.StartsWith(sAbsThis.GetData()))
    {
      Clear();
      const char* szStart = &sAbsBase.GetData()[sAbsThis.GetElementCount()];

      while (*szStart != '\0')
      {
        if (*szStart == '/')
          Append("../");

        ++szStart;
      }

      return PL_SUCCESS;
    }
    else
      sAbsThis.Shrink(0, 1);
  }

  const plUInt32 uiMinLen = plMath::Min(sAbsBase.GetElementCount(), sAbsThis.GetElementCount());

  plInt32 iSame = uiMinLen - 1;
  for (; iSame >= 0; --iSame)
  {
    if (sAbsBase.GetData()[iSame] != '/')
      continue;

    // We need to check here if sAbsThis starts with sAbsBase in the range[0, iSame + 1]. However, we can't compare the first N bytes because those might not be a valid utf8 substring in absBase.
    // Thus we can't use IsEqualN_NoCase as N would need to be the number of characters, not bytes. Computing the number of characters in absBase would mean iterating the string twice.
    // As an alternative, as we know [0, iSame + 1] is a valid utf8 string in sAbsBase we can ask whether absThis starts with that substring.
    if (plStringUtils::StartsWith_NoCase(sAbsThis.GetData(), sAbsBase.GetData(), sAbsThis.GetData() + sAbsThis.GetElementCount(), sAbsBase.GetData() + iSame + 1))
      break;
  }

  if (iSame < 0)
  {
    return PL_FAILURE;
  }

  Clear();

  for (plUInt32 ui = iSame + 1; ui < sAbsBase.GetElementCount(); ++ui)
  {
    if (sAbsBase.GetData()[ui] == '/')
      Append("../");
  }

  if (sAbsThis.GetData()[iSame] == '/')
    ++iSame;

  Append(&(sAbsThis.GetData()[iSame]));

  return PL_SUCCESS;
}

/// An empty folder (zero length) does not contain ANY files.\n
/// A non-existing file-name (zero length) is never in any folder.\n
/// Example:\n
/// IsFileBelowFolder ("", "XYZ") -> always false\n
/// IsFileBelowFolder ("XYZ", "") -> always false\n
/// IsFileBelowFolder ("", "") -> always false\n
bool plStringBuilder::IsPathBelowFolder(const char* szPathToFolder)
{
  PL_ASSERT_DEV(!plStringUtils::IsNullOrEmpty(szPathToFolder), "The given path must not be empty. Because is 'nothing' under the empty path, or 'everything' ?");

  // a non-existing file is never in any folder
  if (IsEmpty())
    return false;

  MakeCleanPath();

  plStringBuilder sBasePath(szPathToFolder);
  sBasePath.MakeCleanPath();

  if (IsEqual_NoCase(sBasePath.GetData()))
    return true;

  if (!sBasePath.EndsWith("/"))
    sBasePath.Append("/");

  return StartsWith_NoCase(sBasePath.GetData());
}

void plStringBuilder::MakePathSeparatorsNative()
{
  const char sep = plPathUtils::OsSpecificPathSeparator;

  MakeCleanPath();
  ReplaceAll("/", plStringView(&sep, 1));
}

void plStringBuilder::RemoveDoubleSlashesInPath()
{
  if (IsEmpty())
    return;

  const char* szReadPos = &m_Data[0];
  char* szCurWritePos = &m_Data[0];

  plInt32 iAllowedSlashes = 2;

  while (*szReadPos != '\0')
  {
    char CurChar = *szReadPos;
    ++szReadPos;

    if (CurChar == '\\')
      CurChar = '/';

    if (CurChar != '/')
      iAllowedSlashes = 1;
    else
    {
      if (iAllowedSlashes > 0)
        --iAllowedSlashes;
      else
        continue;
    }

    *szCurWritePos = CurChar;
    ++szCurWritePos;
  }


  const plUInt32 uiPrevByteCount = m_Data.GetCount();
  const plUInt32 uiNewByteCount = (plUInt32)(szCurWritePos - &m_Data[0]) + 1;

  PL_IGNORE_UNUSED(uiPrevByteCount);
  PL_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
    uiPrevByteCount, uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  *szCurWritePos = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}


void plStringBuilder::ReadAll(plStreamReader& inout_stream)
{
  Clear();

  plHybridArray<plUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  plUInt8 Temp[1024];

  while (true)
  {
    const plUInt32 uiRead = (plUInt32)inout_stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(plArrayPtr<plUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  *this = (const char*)&Bytes[0];
}

void plStringBuilder::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

void plStringBuilder::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  const char* szNewStart = GetData();
  const char* szNewEnd = GetData() + GetElementCount();
  plStringUtils::Trim(szNewStart, szNewEnd, szTrimCharsStart, szTrimCharsEnd);
  Shrink(plStringUtils::GetCharacterCount(GetData(), szNewStart), plStringUtils::GetCharacterCount(szNewEnd, GetData() + GetElementCount()));
}

bool plStringBuilder::TrimWordStart(plStringView sWord)
{
  const bool bTrimAll = false;

  bool trimmed = false;

  do
  {
    if (!sWord.IsEmpty() && StartsWith_NoCase(sWord))
    {
      Shrink(plStringUtils::GetCharacterCount(sWord.GetStartPointer(), sWord.GetEndPointer()), 0);
      trimmed = true;
    }

  } while (bTrimAll);

  return trimmed;
}

bool plStringBuilder::TrimWordEnd(plStringView sWord)
{
  const bool bTrimAll = false;

  bool trimmed = false;

  do
  {

    if (!sWord.IsEmpty() && EndsWith_NoCase(sWord))
    {
      Shrink(0, plStringUtils::GetCharacterCount(sWord.GetStartPointer(), sWord.GetEndPointer()));
      trimmed = true;
    }

  } while (bTrimAll);

  return trimmed;
}

void plStringBuilder::SetFormat(const plFormatString& string)
{
  Clear();
  plStringView sText = string.GetText(*this);

  // this is for the case that GetText does not use the plStringBuilder as temp storage
  if (sText.GetStartPointer() != GetData())
    *this = sText;
}

void plStringBuilder::AppendFormat(const plFormatString& string)
{
  plStringBuilder tmp;
  plStringView view = string.GetText(tmp);

  Append(view);
}

void plStringBuilder::PrependFormat(const plFormatString& string)
{
  plStringBuilder tmp;

  Prepend(string.GetText(tmp));
}

void plStringBuilder::SetPrintf(const char* szUtf8Format, ...)
{
  va_list args;
  va_start(args, szUtf8Format);

  SetPrintfArgs(szUtf8Format, args);

  va_end(args);
}

#if PL_ENABLED(PL_INTEROP_STL_STRINGS)
plStringBuilder::plStringBuilder(const std::string_view& rhs, plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();

  *this = rhs;
}

plStringBuilder::plStringBuilder(const std::string& rhs, plAllocator* pAllocator)
  : m_Data(pAllocator)
{
  AppendTerminator();

  *this = rhs;
}

void plStringBuilder::operator=(const std::string_view& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    *this = plStringView(rhs.data(), rhs.data() + rhs.size());
  }
}

void plStringBuilder::operator=(const std::string& rhs)
{
  if (rhs.empty())
  {
    Clear();
  }
  else
  {
    *this = plStringView(rhs.data(), rhs.data() + rhs.size());
  }
}
#endif
