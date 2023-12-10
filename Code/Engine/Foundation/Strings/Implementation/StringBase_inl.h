#pragma once

template <typename Derived>
PLASMA_ALWAYS_INLINE const char* plStringBase<Derived>::InternalGetData() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData();
}

template <typename Derived>
PLASMA_ALWAYS_INLINE const char* plStringBase<Derived>::InternalGetDataEnd() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetData() + pDerived->GetElementCount();
}

template <typename Derived>
PLASMA_ALWAYS_INLINE plUInt32 plStringBase<Derived>::InternalGetElementCount() const
{
  const Derived* pDerived = static_cast<const Derived*>(this);
  return pDerived->GetElementCount();
}

template <typename Derived>
PLASMA_ALWAYS_INLINE bool plStringBase<Derived>::IsEmpty() const
{
  return plStringUtils::IsNullOrEmpty(InternalGetData()) || (InternalGetData() == InternalGetDataEnd());
}

template <typename Derived>
bool plStringBase<Derived>::StartsWith(plStringView sStartsWith) const
{
  return plStringUtils::StartsWith(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool plStringBase<Derived>::StartsWith_NoCase(plStringView sStartsWith) const
{
  return plStringUtils::StartsWith_NoCase(InternalGetData(), sStartsWith.GetStartPointer(), InternalGetDataEnd(), sStartsWith.GetEndPointer());
}

template <typename Derived>
bool plStringBase<Derived>::EndsWith(plStringView sEndsWith) const
{
  return plStringUtils::EndsWith(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
bool plStringBase<Derived>::EndsWith_NoCase(plStringView sEndsWith) const
{
  return plStringUtils::EndsWith_NoCase(InternalGetData(), sEndsWith.GetStartPointer(), InternalGetDataEnd(), sEndsWith.GetEndPointer());
}

template <typename Derived>
const char* plStringBase<Derived>::FindSubString(plStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  PLASMA_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
const char* plStringBase<Derived>::FindSubString_NoCase(plStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  PLASMA_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* plStringBase<Derived>::FindLastSubString(plStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  PLASMA_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()),
    "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindLastSubString(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* plStringBase<Derived>::FindLastSubString_NoCase(plStringView sStringToFind, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetDataEnd();

  PLASMA_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()),
    "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindLastSubString_NoCase(InternalGetData(), sStringToFind.GetStartPointer(), szStartSearchAt, InternalGetDataEnd(), sStringToFind.GetEndPointer());
}

template <typename Derived>
inline const char* plStringBase<Derived>::FindWholeWord(const char* szSearchFor, plStringUtils::PLASMA_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  PLASMA_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
inline const char* plStringBase<Derived>::FindWholeWord_NoCase(const char* szSearchFor, plStringUtils::PLASMA_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /* = nullptr */) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = InternalGetData();

  PLASMA_ASSERT_DEV((szStartSearchAt >= InternalGetData()) && (szStartSearchAt <= InternalGetDataEnd()), "The given pointer to start searching at is not inside this strings valid range.");

  return plStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, InternalGetDataEnd());
}

template <typename Derived>
plInt32 plStringBase<Derived>::Compare(plStringView sOther) const
{
  return plStringUtils::Compare(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
plInt32 plStringBase<Derived>::CompareN(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::CompareN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
plInt32 plStringBase<Derived>::Compare_NoCase(plStringView sOther) const
{
  return plStringUtils::Compare_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
plInt32 plStringBase<Derived>::CompareN_NoCase(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::CompareN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool plStringBase<Derived>::IsEqual(plStringView sOther) const
{
  return plStringUtils::IsEqual(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool plStringBase<Derived>::IsEqualN(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::IsEqualN(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool plStringBase<Derived>::IsEqual_NoCase(plStringView sOther) const
{
  return plStringUtils::IsEqual_NoCase(InternalGetData(), sOther.GetStartPointer(), InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
bool plStringBase<Derived>::IsEqualN_NoCase(plStringView sOther, plUInt32 uiCharsToCompare) const
{
  return plStringUtils::IsEqualN_NoCase(InternalGetData(), sOther.GetStartPointer(), uiCharsToCompare, InternalGetDataEnd(), sOther.GetEndPointer());
}

template <typename Derived>
const char* plStringBase<Derived>::ComputeCharacterPosition(plUInt32 uiCharacterIndex) const
{
  const char* pos = InternalGetData();
  plUnicodeUtils::MoveToNextUtf8(pos, InternalGetDataEnd(), uiCharacterIndex);
  return pos;
}

template <typename Derived>
typename plStringBase<Derived>::iterator plStringBase<Derived>::GetIteratorFront() const
{
  return begin(*this);
}

template <typename Derived>
typename plStringBase<Derived>::reverse_iterator plStringBase<Derived>::GetIteratorBack() const
{
  return rbegin(*this);
}

template <typename DerivedLhs, typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator==(const plStringBase<DerivedLhs>& lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.IsEqual(rhs.GetView());
}

template <typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator==(const char* lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE bool operator==(const plStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.IsEqual(rhs);
}

template <typename DerivedLhs, typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator!=(const plStringBase<DerivedLhs>& lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return !lhs.IsEqual(rhs);
}

template <typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator!=(const char* lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return !rhs.IsEqual(lhs);
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE bool operator!=(const plStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return !lhs.IsEqual(rhs);
}

template <typename DerivedLhs, typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator<(const plStringBase<DerivedLhs>& lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.Compare(rhs) < 0;
}

template <typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator<(const char* lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) > 0;
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE bool operator<(const plStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) < 0;
}

template <typename DerivedLhs, typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator>(const plStringBase<DerivedLhs>& lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return lhs.Compare(rhs) > 0;
}

template <typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator>(const char* lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) < 0;
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE bool operator>(const plStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) > 0;
}

template <typename DerivedLhs, typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator<=(const plStringBase<DerivedLhs>& lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return plStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) <= 0;
}

template <typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator<=(const char* lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) >= 0;
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE bool operator<=(const plStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) <= 0;
}

template <typename DerivedLhs, typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator>=(const plStringBase<DerivedLhs>& lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return plStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) >= 0;
}

template <typename DerivedRhs>
PLASMA_ALWAYS_INLINE bool operator>=(const char* lhs, const plStringBase<DerivedRhs>& rhs) // [tested]
{
  return rhs.Compare(lhs) <= 0;
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE bool operator>=(const plStringBase<DerivedLhs>& lhs, const char* rhs) // [tested]
{
  return lhs.Compare(rhs) >= 0;
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE plStringBase<DerivedLhs>::operator plStringView() const
{
  return plStringView(InternalGetData(), InternalGetElementCount());
}

template <typename Derived>
PLASMA_ALWAYS_INLINE plStringView plStringBase<Derived>::GetView() const
{
  return plStringView(InternalGetData(), InternalGetElementCount());
}

template <typename DerivedLhs>
PLASMA_ALWAYS_INLINE plStringBase<DerivedLhs>::operator std::string_view() const
{
  return std::string_view(InternalGetData(), static_cast<size_t>(InternalGetElementCount()));
}

template <typename Derived>
PLASMA_ALWAYS_INLINE std::string_view plStringBase<Derived>::GetAsStdView() const
{
  return std::string_view(InternalGetData(), static_cast<size_t>(InternalGetElementCount()));
}

template <typename Derived>
template <typename Container>
void plStringBase<Derived>::Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 /*= nullptr*/, const char* szSeparator3 /*= nullptr*/, const char* szSeparator4 /*= nullptr*/, const char* szSeparator5 /*= nullptr*/, const char* szSeparator6 /*= nullptr*/) const
{
  GetView().Split(bReturnEmptyStrings, ref_output, szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6);
}

template <typename Derived>
plStringView plStringBase<Derived>::GetRootedPathRootName() const
{
  return GetView().GetRootedPathRootName();
}

template <typename Derived>
bool plStringBase<Derived>::IsRootedPath() const
{
  return GetView().IsRootedPath();
}

template <typename Derived>
bool plStringBase<Derived>::IsRelativePath() const
{
  return GetView().IsRelativePath();
}

template <typename Derived>
bool plStringBase<Derived>::IsAbsolutePath() const
{
  return GetView().IsAbsolutePath();
}

template <typename Derived>
plStringView plStringBase<Derived>::GetFileDirectory() const
{
  return GetView().GetFileDirectory();
}

template <typename Derived>
plStringView plStringBase<Derived>::GetFileNameAndExtension() const
{
  return GetView().GetFileNameAndExtension();
}

template <typename Derived>
plStringView plStringBase<Derived>::GetFileName() const
{
  return GetView().GetFileName();
}

template <typename Derived>
plStringView plStringBase<Derived>::GetFileExtension() const
{
  return GetView().GetFileExtension();
}

template <typename Derived>
bool plStringBase<Derived>::HasExtension(plStringView sExtension) const
{
  return GetView().HasExtension(sExtension);
}

template <typename Derived>
bool plStringBase<Derived>::HasAnyExtension() const
{
  return GetView().HasAnyExtension();
}
