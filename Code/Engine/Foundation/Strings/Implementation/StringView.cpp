#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

plUInt32 plStringView::GetCharacter() const
{
  if (!IsValid())
    return 0;

  return plUnicodeUtils::ConvertUtf8ToUtf32(m_pStart);
}

const char* plStringView::GetData(plStringBuilder& ref_sTempStorage) const
{
  ref_sTempStorage = *this;
  return ref_sTempStorage.GetData();
}

bool plStringView::IsEqualN(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::IsEqualN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

bool plStringView::IsEqualN_NoCase(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::IsEqualN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

plInt32 plStringView::Compare(plStringView sOther) const
{
  return plStringUtils::Compare(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

plInt32 plStringView::CompareN(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::CompareN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

plInt32 plStringView::Compare_NoCase(plStringView sOther) const
{
  return plStringUtils::Compare_NoCase(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

plInt32 plStringView::CompareN_NoCase(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::CompareN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

const char* plStringView::ComputeCharacterPosition(plUInt32 uiCharacterIndex) const
{
  const char* pos = GetStartPointer();
  if (plUnicodeUtils::MoveToNextUtf8(pos, GetEndPointer(), uiCharacterIndex).Failed())
    return nullptr;

  return pos;
}

const char* plStringView::FindSubString(plStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  PL_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* plStringView::FindSubString_NoCase(plStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  PL_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* plStringView::FindLastSubString(plStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  PL_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindLastSubString(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* plStringView::FindLastSubString_NoCase(plStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  PL_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindLastSubString_NoCase(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* plStringView::FindWholeWord(const char* szSearchFor, plStringUtils::PL_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  PL_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

const char* plStringView::FindWholeWord_NoCase(const char* szSearchFor, plStringUtils::PL_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  PL_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

void plStringView::Shrink(plUInt32 uiShrinkCharsFront, plUInt32 uiShrinkCharsBack)
{
  while (IsValid() && (uiShrinkCharsFront > 0))
  {
    if (plUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd, 1).Failed())
    {
      *this = {};
      return;
    }

    --uiShrinkCharsFront;
  }

  while (IsValid() && (uiShrinkCharsBack > 0))
  {
    if (plUnicodeUtils::MoveToPriorUtf8(m_pEnd, m_pStart, 1).Failed())
    {
      *this = {};
      return;
    }

    --uiShrinkCharsBack;
  }
}

plStringView plStringView::GetShrunk(plUInt32 uiShrinkCharsFront, plUInt32 uiShrinkCharsBack) const
{
  plStringView tmp = *this;
  tmp.Shrink(uiShrinkCharsFront, uiShrinkCharsBack);
  return tmp;
}

plStringView plStringView::GetSubString(plUInt32 uiFirstCharacter, plUInt32 uiNumCharacters) const
{
  if (!IsValid())
  {
    return {};
  }

  const char* pStart = m_pStart;
  if (plUnicodeUtils::MoveToNextUtf8(pStart, m_pEnd, uiFirstCharacter).Failed() || pStart == m_pEnd)
  {
    return {};
  }

  const char* pEnd = pStart;
  plUnicodeUtils::MoveToNextUtf8(pEnd, m_pEnd, uiNumCharacters).IgnoreResult(); // if it fails, it just points to the end

  return plStringView(pStart, pEnd);
}

void plStringView::ChopAwayFirstCharacterUtf8()
{
  if (IsValid())
  {
    plUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd, 1).AssertSuccess();
  }
}

void plStringView::ChopAwayFirstCharacterAscii()
{
  if (IsValid())
  {
    PL_ASSERT_DEBUG(plUnicodeUtils::IsASCII(*m_pStart), "ChopAwayFirstCharacterAscii() was called on a non-ASCII character.");

    m_pStart += 1;
  }
}

bool plStringView::TrimWordStart(plStringView sWord)
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

bool plStringView::TrimWordEnd(plStringView sWord)
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

plStringView::iterator plStringView::GetIteratorFront() const
{
  return begin(*this);
}

plStringView::reverse_iterator plStringView::GetIteratorBack() const
{
  return rbegin(*this);
}

bool plStringView::HasAnyExtension() const
{
  return plPathUtils::HasAnyExtension(*this);
}

bool plStringView::HasExtension(plStringView sExtension) const
{
  return plPathUtils::HasExtension(*this, sExtension);
}

plStringView plStringView::GetFileExtension() const
{
  return plPathUtils::GetFileExtension(*this);
}

plStringView plStringView::GetFileName() const
{
  return plPathUtils::GetFileName(*this);
}

plStringView plStringView::GetFileNameAndExtension() const
{
  return plPathUtils::GetFileNameAndExtension(*this);
}

plStringView plStringView::GetFileDirectory() const
{
  return plPathUtils::GetFileDirectory(*this);
}

bool plStringView::IsAbsolutePath() const
{
  return plPathUtils::IsAbsolutePath(*this);
}

bool plStringView::IsRelativePath() const
{
  return plPathUtils::IsRelativePath(*this);
}

bool plStringView::IsRootedPath() const
{
  return plPathUtils::IsRootedPath(*this);
}

plStringView plStringView::GetRootedPathRootName() const
{
  return plPathUtils::GetRootedPathRootName(*this);
}

#if PL_ENABLED(PL_INTEROP_STL_STRINGS)
plStringView::plStringView(const std::string_view& rhs)
{
  if (!rhs.empty())
  {
    m_pStart = rhs.data();
    m_pEnd = rhs.data() + rhs.size();
  }
}

plStringView::plStringView(const std::string& rhs)
{
  if (!rhs.empty())
  {
    m_pStart = rhs.data();
    m_pEnd = rhs.data() + rhs.size();
  }
}

std::string_view plStringView::GetAsStdView() const
{
  return std::string_view(GetStartPointer(), static_cast<size_t>(GetElementCount()));
}

plStringView::operator std::string_view() const
{
  return GetAsStdView();
}
#endif
