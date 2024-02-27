#pragma once

PL_ALWAYS_INLINE constexpr plStringView::plStringView() = default;

PL_ALWAYS_INLINE plStringView::plStringView(char* pStart)
  : m_pStart(pStart)
  , m_uiElementCount(plStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
constexpr PL_ALWAYS_INLINE plStringView::plStringView(T pStart, typename std::enable_if<std::is_same<T, const char*>::value, int>::type*)
  : m_pStart(pStart)
  , m_uiElementCount(plStringUtils::GetStringElementCount(pStart))
{
}

template <typename T>
PL_ALWAYS_INLINE plStringView::plStringView(const T&& str, typename std::enable_if<std::is_same<T, const char*>::value == false && std::is_convertible<T, const char*>::value, int>::type*)
{
  m_pStart = str;
  m_uiElementCount = plStringUtils::GetStringElementCount(m_pStart);
}

PL_ALWAYS_INLINE plStringView::plStringView(const char* pStart, const char* pEnd)
{
  PL_ASSERT_DEV(pStart <= pEnd, "It should start BEFORE it ends.");

  m_pStart = pStart;
  m_uiElementCount = static_cast<plUInt32>(pEnd - pStart);
}

constexpr PL_ALWAYS_INLINE plStringView::plStringView(const char* pStart, plUInt32 uiLength)
  : m_pStart(pStart)
  , m_uiElementCount(uiLength)
{
}

template <size_t N>
PL_ALWAYS_INLINE plStringView::plStringView(const char (&str)[N])
  : m_pStart(str)
  , m_uiElementCount(N - 1)
{
  static_assert(N > 0, "Not a string literal");
  PL_ASSERT_DEBUG(str[N - 1] == '\0', "Not a string literal. Manually cast to 'const char*' if you are trying to pass a const char fixed size array.");
}

template <size_t N>
PL_ALWAYS_INLINE plStringView::plStringView(char (&str)[N])
{
  m_pStart = str;
  m_uiElementCount = plStringUtils::GetStringElementCount(str, str + N);
}

inline void plStringView::operator++()
{
  if (!IsValid())
    return;

  const char* pEnd = m_pStart + m_uiElementCount;
  plUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd).IgnoreResult(); // if it fails, the string is just empty
  m_uiElementCount = static_cast<plUInt32>(pEnd - m_pStart);
}

inline void plStringView::operator+=(plUInt32 d)
{
  const char* pEnd = m_pStart + m_uiElementCount;
  plUnicodeUtils::MoveToNextUtf8(m_pStart, pEnd, d).IgnoreResult(); // if it fails, the string is just empty
  m_uiElementCount = static_cast<plUInt32>(pEnd - m_pStart);
}

PL_ALWAYS_INLINE bool plStringView::IsValid() const
{
  return (m_pStart != nullptr) && (m_uiElementCount > 0);
}

PL_ALWAYS_INLINE void plStringView::SetStartPosition(const char* szCurPos)
{
  PL_ASSERT_DEV((szCurPos >= m_pStart) && (szCurPos <= m_pStart + m_uiElementCount), "New start position must still be inside the view's range.");

  const char* pEnd = m_pStart + m_uiElementCount;
  m_pStart = szCurPos;
  m_uiElementCount = static_cast<plUInt32>(pEnd - m_pStart);
}

PL_ALWAYS_INLINE bool plStringView::IsEmpty() const
{
  return m_uiElementCount == 0;
}

PL_ALWAYS_INLINE bool plStringView::IsEqual(plStringView sOther) const
{
  return plStringUtils::IsEqual(m_pStart, sOther.GetStartPointer(), m_pStart + m_uiElementCount, sOther.GetEndPointer());
}

PL_ALWAYS_INLINE bool plStringView::IsEqual_NoCase(plStringView sOther) const
{
  return plStringUtils::IsEqual_NoCase(m_pStart, sOther.GetStartPointer(), m_pStart + m_uiElementCount, sOther.GetEndPointer());
}

PL_ALWAYS_INLINE bool plStringView::StartsWith(plStringView sStartsWith) const
{
  return plStringUtils::StartsWith(m_pStart, sStartsWith.GetStartPointer(), m_pStart + m_uiElementCount, sStartsWith.GetEndPointer());
}

PL_ALWAYS_INLINE bool plStringView::StartsWith_NoCase(plStringView sStartsWith) const
{
  return plStringUtils::StartsWith_NoCase(m_pStart, sStartsWith.GetStartPointer(), m_pStart + m_uiElementCount, sStartsWith.GetEndPointer());
}

PL_ALWAYS_INLINE bool plStringView::EndsWith(plStringView sEndsWith) const
{
  return plStringUtils::EndsWith(m_pStart, sEndsWith.GetStartPointer(), m_pStart + m_uiElementCount, sEndsWith.GetEndPointer());
}

PL_ALWAYS_INLINE bool plStringView::EndsWith_NoCase(plStringView sEndsWith) const
{
  return plStringUtils::EndsWith_NoCase(m_pStart, sEndsWith.GetStartPointer(), m_pStart + m_uiElementCount, sEndsWith.GetEndPointer());
}

PL_ALWAYS_INLINE void plStringView::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

PL_ALWAYS_INLINE void plStringView::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  if (IsValid())
  {
    const char* pEnd = m_pStart + m_uiElementCount;
    plStringUtils::Trim(m_pStart, pEnd, szTrimCharsStart, szTrimCharsEnd);
    m_uiElementCount = static_cast<plUInt32>(pEnd - m_pStart);
  }
}

constexpr PL_ALWAYS_INLINE plStringView operator"" _plsv(const char* pString, size_t uiLen)
{
  return plStringView(pString, static_cast<plUInt32>(uiLen));
}

template <typename Container>
void plStringView::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  ref_output.Clear();

  if (IsEmpty())
    return;

  const plUInt32 uiParams = 6;

  const plStringView seps[uiParams] = {szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6};

  const char* szReadPos = GetStartPointer();

  while (true)
  {
    const char* szFoundPos = plUnicodeUtils::GetMaxStringEnd<char>();
    plUInt32 uiFoundSeparator = 0;

    for (plUInt32 i = 0; i < uiParams; ++i)
    {
      const char* szFound = plStringUtils::FindSubString(szReadPos, seps[i].GetStartPointer(), GetEndPointer(), seps[i].GetEndPointer());

      if ((szFound != nullptr) && (szFound < szFoundPos))
      {
        szFoundPos = szFound;
        uiFoundSeparator = i;
      }
    }

    // nothing found
    if (szFoundPos == plUnicodeUtils::GetMaxStringEnd<char>())
    {
      const plUInt32 uiLen = plStringUtils::GetStringElementCount(szReadPos, GetEndPointer());

      if (bReturnEmptyStrings || (uiLen > 0))
        ref_output.PushBack(plStringView(szReadPos, szReadPos + uiLen));

      return;
    }

    if (bReturnEmptyStrings || (szFoundPos > szReadPos))
      ref_output.PushBack(plStringView(szReadPos, szFoundPos));

    szReadPos = szFoundPos + seps[uiFoundSeparator].GetElementCount();
  }
}

PL_ALWAYS_INLINE bool operator==(plStringView lhs, plStringView rhs)
{
  return lhs.IsEqual(rhs);
}

#if PL_DISABLED(PL_USE_CPP20_OPERATORS)

PL_ALWAYS_INLINE bool operator!=(plStringView lhs, plStringView rhs)
{
  return !lhs.IsEqual(rhs);
}

#endif

PL_ALWAYS_INLINE bool operator<(plStringView lhs, plStringView rhs)
{
  return lhs.Compare(rhs) < 0;
}

PL_ALWAYS_INLINE bool operator<=(plStringView lhs, plStringView rhs)
{
  return lhs.Compare(rhs) <= 0;
}

PL_ALWAYS_INLINE bool operator>(plStringView lhs, plStringView rhs)
{
  return lhs.Compare(rhs) > 0;
}

PL_ALWAYS_INLINE bool operator>=(plStringView lhs, plStringView rhs)
{
  return lhs.Compare(rhs) >= 0;
}
