#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

template <typename STRING>
void TestConstruction(const STRING& value, const char* szStart, const char* szEnd)
{
  plStringUtf8 sUtf8(L"A単語F");
  PLASMA_TEST_BOOL(value.IsEqual(sUtf8.GetData()));
  const bool bEqualForwardItTypes = plConversionTest<typename STRING::iterator, typename STRING::const_iterator>::sameType == 1;
  PLASMA_CHECK_AT_COMPILETIME_MSG(
    bEqualForwardItTypes, "As the string iterator is read-only, both const and non-const versions should be the same type.");
  const bool bEqualReverseItTypes = plConversionTest<typename STRING::reverse_iterator, typename STRING::const_reverse_iterator>::sameType == 1;
  PLASMA_CHECK_AT_COMPILETIME_MSG(
    bEqualReverseItTypes, "As the reverse string iterator is read-only, both const and non-const versions should be the same type.");

  typename STRING::iterator itInvalid;
  PLASMA_TEST_BOOL(!itInvalid.IsValid());
  typename STRING::reverse_iterator itInvalidR;
  PLASMA_TEST_BOOL(!itInvalidR.IsValid());

  // Begin
  const typename STRING::iterator itBegin = begin(value);
  PLASMA_TEST_BOOL(itBegin == value.GetIteratorFront());
  PLASMA_TEST_BOOL(itBegin.IsValid());
  PLASMA_TEST_BOOL(itBegin == itBegin);
  PLASMA_TEST_BOOL(itBegin.GetData() == szStart);
  PLASMA_TEST_BOOL(itBegin.GetCharacter() == plUnicodeUtils::ConvertUtf8ToUtf32("A"));
  PLASMA_TEST_BOOL(*itBegin == plUnicodeUtils::ConvertUtf8ToUtf32("A"));

  // End
  const typename STRING::iterator itEnd = end(value);
  PLASMA_TEST_BOOL(!itEnd.IsValid());
  PLASMA_TEST_BOOL(itEnd == itEnd);
  PLASMA_TEST_BOOL(itBegin != itEnd);
  PLASMA_TEST_BOOL(itEnd.GetData() == szEnd);
  PLASMA_TEST_BOOL(itEnd.GetCharacter() == 0);
  PLASMA_TEST_BOOL(*itEnd == 0);

  // RBegin
  const typename STRING::reverse_iterator itBeginR = rbegin(value);
  PLASMA_TEST_BOOL(itBeginR == value.GetIteratorBack());
  PLASMA_TEST_BOOL(itBeginR.IsValid());
  PLASMA_TEST_BOOL(itBeginR == itBeginR);
  const char* szEndPrior = szEnd;
  plUnicodeUtils::MoveToPriorUtf8(szEndPrior);
  PLASMA_TEST_BOOL(itBeginR.GetData() == szEndPrior);
  PLASMA_TEST_BOOL(itBeginR.GetCharacter() == plUnicodeUtils::ConvertUtf8ToUtf32("F"));
  PLASMA_TEST_BOOL(*itBeginR == plUnicodeUtils::ConvertUtf8ToUtf32("F"));

  // REnd
  const typename STRING::reverse_iterator itEndR = rend(value);
  PLASMA_TEST_BOOL(!itEndR.IsValid());
  PLASMA_TEST_BOOL(itEndR == itEndR);
  PLASMA_TEST_BOOL(itBeginR != itEndR);
  PLASMA_TEST_BOOL(itEndR.GetData() == nullptr); // Position before first character is not a valid ptr, so it is set to nullptr.
  PLASMA_TEST_BOOL(itEndR.GetCharacter() == 0);
  PLASMA_TEST_BOOL(*itEndR == 0);
}

template <typename STRING, typename IT>
void TestIteratorBegin(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itBegin = it;
  --itBegin;
  itBegin -= 4;
  PLASMA_TEST_BOOL(itBegin == it);
  PLASMA_TEST_BOOL(itBegin - 2 == it);

  // Prefix / Postfix
  PLASMA_TEST_BOOL(itBegin + 2 != it);
  PLASMA_TEST_BOOL(itBegin++ == it);
  PLASMA_TEST_BOOL(itBegin-- != it);
  itBegin = it;
  PLASMA_TEST_BOOL(++itBegin != it);
  PLASMA_TEST_BOOL(--itBegin == it);

  // Misc
  itBegin = it;
  PLASMA_TEST_BOOL(it + 2 == ++(++itBegin));
  itBegin -= 1;
  PLASMA_TEST_BOOL(itBegin == it + 1);
  itBegin -= 0;
  PLASMA_TEST_BOOL(itBegin == it + 1);
  itBegin += 0;
  PLASMA_TEST_BOOL(itBegin == it + 1);
  itBegin += -1;
  PLASMA_TEST_BOOL(itBegin == it);
}

template <typename STRING, typename IT>
void TestIteratorEnd(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itEnd = it;
  ++itEnd;
  itEnd += 4;
  PLASMA_TEST_BOOL(itEnd == it);
  PLASMA_TEST_BOOL(itEnd + 2 == it);

  // Prefix / Postfix
  PLASMA_TEST_BOOL(itEnd - 2 != it);
  PLASMA_TEST_BOOL(itEnd-- == it);
  PLASMA_TEST_BOOL(itEnd++ != it);
  itEnd = it;
  PLASMA_TEST_BOOL(--itEnd != it);
  PLASMA_TEST_BOOL(++itEnd == it);

  // Misc
  itEnd = it;
  PLASMA_TEST_BOOL(it - 2 == --(--itEnd));
  itEnd += 1;
  PLASMA_TEST_BOOL(itEnd == it - 1);
  itEnd += 0;
  PLASMA_TEST_BOOL(itEnd == it - 1);
  itEnd -= 0;
  PLASMA_TEST_BOOL(itEnd == it - 1);
  itEnd -= -1;
  PLASMA_TEST_BOOL(itEnd == it);
}

template <typename STRING>
void TestOperators(const STRING& value, const char* szStart, const char* szEnd)
{
  plStringUtf8 sUtf8(L"A単語F");
  PLASMA_TEST_BOOL(value.IsEqual(sUtf8.GetData()));

  // Begin
  typename STRING::iterator itBegin = begin(value);
  TestIteratorBegin(value, itBegin);

  // End
  typename STRING::iterator itEnd = end(value);
  TestIteratorEnd(value, itEnd);

  // RBegin
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  TestIteratorBegin(value, itBeginR);

  // REnd
  typename STRING::reverse_iterator itEndR = rend(value);
  TestIteratorEnd(value, itEndR);
}

template <typename STRING>
void TestLoops(const STRING& value, const char* szStart, const char* szEnd)
{
  plStringUtf8 sUtf8(L"A単語F");
  plUInt32 characters[] = {plUnicodeUtils::ConvertUtf8ToUtf32(plStringUtf8(L"A").GetData()),
    plUnicodeUtils::ConvertUtf8ToUtf32(plStringUtf8(L"単").GetData()), plUnicodeUtils::ConvertUtf8ToUtf32(plStringUtf8(L"語").GetData()),
    plUnicodeUtils::ConvertUtf8ToUtf32(plStringUtf8(L"F").GetData())};

  // Forward
  plInt32 iIndex = 0;
  for (plUInt32 character : value)
  {
    PLASMA_TEST_INT(characters[iIndex], character);
    ++iIndex;
  }
  PLASMA_TEST_INT(iIndex, 4);

  typename STRING::iterator itBegin = begin(value);
  typename STRING::iterator itEnd = end(value);
  iIndex = 0;
  for (auto it = itBegin; it != itEnd; ++it)
  {
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(characters[iIndex], it.GetCharacter());
    PLASMA_TEST_INT(characters[iIndex], *it);
    PLASMA_TEST_BOOL(it.GetData() >= szStart);
    PLASMA_TEST_BOOL(it.GetData() < szEnd);
    ++iIndex;
  }
  PLASMA_TEST_INT(iIndex, 4);

  // Reverse
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  typename STRING::reverse_iterator itEndR = rend(value);
  iIndex = 3;
  for (auto it = itBeginR; it != itEndR; ++it)
  {
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(characters[iIndex], it.GetCharacter());
    PLASMA_TEST_INT(characters[iIndex], *it);
    PLASMA_TEST_BOOL(it.GetData() >= szStart);
    PLASMA_TEST_BOOL(it.GetData() < szEnd);
    --iIndex;
  }
  PLASMA_TEST_INT(iIndex, -1);
}

PLASMA_CREATE_SIMPLE_TEST(Strings, StringIterator)
{
  plStringUtf8 sUtf8(L"_A単語F_");
  plStringBuilder sTestStringBuilder = sUtf8.GetData();
  sTestStringBuilder.Shrink(1, 1);
  plString sTextString = sTestStringBuilder.GetData();

  plStringView view(sUtf8.GetData());
  view.Shrink(1, 1);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Construction")
  {
    TestConstruction<plString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestConstruction<plStringBuilder>(
      sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestConstruction<plStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    TestOperators<plString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestOperators<plStringBuilder>(
      sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestOperators<plStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Loops")
  {
    TestLoops<plString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestLoops<plStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestLoops<plStringView>(view, view.GetStartPointer(), view.GetEndPointer());
  }
}
