#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

PLASMA_CREATE_SIMPLE_TEST(Strings, StringBase)
{
  // These tests need not be very through, as plStringBase only passes through to plStringUtil
  // which has been tested elsewhere already.
  // Here it is only assured that plStringBases passes its own pointers properly through,
  // such that the plStringUtil functions are called correctly.

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEmpty")
  {
    plStringView it(nullptr);
    PLASMA_TEST_BOOL(it.IsEmpty());

    plStringView it2("");
    PLASMA_TEST_BOOL(it2.IsEmpty());

    plStringView it3(nullptr, nullptr);
    PLASMA_TEST_BOOL(it3.IsEmpty());

    const char* sz = "abcdef";

    plStringView it4(sz, sz);
    PLASMA_TEST_BOOL(it4.IsEmpty());

    plStringView it5(sz, sz + 1);
    PLASMA_TEST_BOOL(!it5.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StartsWith")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.StartsWith("abc"));
    PLASMA_TEST_BOOL(it.StartsWith("abcdef"));
    PLASMA_TEST_BOOL(it.StartsWith("")); // empty strings always return true

    plStringView it2(sz + 3);

    PLASMA_TEST_BOOL(it2.StartsWith("def"));
    PLASMA_TEST_BOOL(it2.StartsWith(""));

    plStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    PLASMA_TEST_BOOL(it3.StartsWith("d"));
    PLASMA_TEST_BOOL(!it3.StartsWith("de"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StartsWith_NoCase")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.StartsWith_NoCase("ABC"));
    PLASMA_TEST_BOOL(it.StartsWith_NoCase("abcDEF"));
    PLASMA_TEST_BOOL(it.StartsWith_NoCase("")); // empty strings always return true

    plStringView it2(sz + 3);

    PLASMA_TEST_BOOL(it2.StartsWith_NoCase("DEF"));
    PLASMA_TEST_BOOL(it2.StartsWith_NoCase(""));

    plStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    PLASMA_TEST_BOOL(it3.StartsWith_NoCase("D"));
    PLASMA_TEST_BOOL(!it3.StartsWith_NoCase("DE"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EndsWith")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.EndsWith("def"));
    PLASMA_TEST_BOOL(it.EndsWith("abcdef"));
    PLASMA_TEST_BOOL(it.EndsWith("")); // empty strings always return true

    plStringView it2(sz + 3);

    PLASMA_TEST_BOOL(it2.EndsWith("def"));
    PLASMA_TEST_BOOL(it2.EndsWith(""));

    plStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    PLASMA_TEST_BOOL(it3.EndsWith("d"));
    PLASMA_TEST_BOOL(!it3.EndsWith("cd"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EndsWith_NoCase")
  {
    const char* sz = "ABCDEF";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.EndsWith_NoCase("def"));
    PLASMA_TEST_BOOL(it.EndsWith_NoCase("abcdef"));
    PLASMA_TEST_BOOL(it.EndsWith_NoCase("")); // empty strings always return true

    plStringView it2(sz + 3);

    PLASMA_TEST_BOOL(it2.EndsWith_NoCase("def"));
    PLASMA_TEST_BOOL(it2.EndsWith_NoCase(""));

    plStringView it3(sz + 2, sz + 4);
    it3.SetStartPosition(sz + 3);

    PLASMA_TEST_BOOL(it3.EndsWith_NoCase("d"));
    PLASMA_TEST_BOOL(!it3.EndsWith_NoCase("cd"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindSubString")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.FindSubString("abcdef") == sz);
    PLASMA_TEST_BOOL(it.FindSubString("abc") == sz);
    PLASMA_TEST_BOOL(it.FindSubString("def") == sz + 3);
    PLASMA_TEST_BOOL(it.FindSubString("cd") == sz + 2);
    PLASMA_TEST_BOOL(it.FindSubString("") == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString("g") == nullptr);

    PLASMA_TEST_BOOL(it.FindSubString("abcdef", sz) == sz);
    PLASMA_TEST_BOOL(it.FindSubString("abcdef", sz + 1) == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString("def", sz + 2) == sz + 3);
    PLASMA_TEST_BOOL(it.FindSubString("def", sz + 3) == sz + 3);
    PLASMA_TEST_BOOL(it.FindSubString("def", sz + 4) == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString("", sz + 3) == nullptr);

    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.FindSubString("abcdef") == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString("abc") == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString("de") == sz + 3);
    PLASMA_TEST_BOOL(it2.FindSubString("cd") == sz + 2);
    PLASMA_TEST_BOOL(it2.FindSubString("") == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString("g") == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.FindSubString_NoCase("abcdef") == sz);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("abc") == sz);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("def") == sz + 3);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("cd") == sz + 2);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("") == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("g") == nullptr);

    PLASMA_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz) == sz);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz + 1) == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("def", sz + 2) == sz + 3);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("def", sz + 3) == sz + 3);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("def", sz + 4) == nullptr);
    PLASMA_TEST_BOOL(it.FindSubString_NoCase("", sz + 3) == nullptr);


    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.FindSubString_NoCase("abcdef") == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString_NoCase("abc") == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString_NoCase("de") == sz + 3);
    PLASMA_TEST_BOOL(it2.FindSubString_NoCase("cd") == sz + 2);
    PLASMA_TEST_BOOL(it2.FindSubString_NoCase("") == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString_NoCase(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it2.FindSubString_NoCase("g") == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindLastSubString")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.FindLastSubString("abcdef") == sz);
    PLASMA_TEST_BOOL(it.FindLastSubString("abc") == sz);
    PLASMA_TEST_BOOL(it.FindLastSubString("def") == sz + 3);
    PLASMA_TEST_BOOL(it.FindLastSubString("cd") == sz + 2);
    PLASMA_TEST_BOOL(it.FindLastSubString("") == nullptr);
    PLASMA_TEST_BOOL(it.FindLastSubString(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it.FindLastSubString("g") == nullptr);

    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.FindLastSubString("abcdef") == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString("abc") == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString("de") == sz + 3);
    PLASMA_TEST_BOOL(it2.FindLastSubString("cd") == sz + 2);
    PLASMA_TEST_BOOL(it2.FindLastSubString("") == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString("g") == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.FindLastSubString_NoCase("abcdef") == sz);
    PLASMA_TEST_BOOL(it.FindLastSubString_NoCase("abc") == sz);
    PLASMA_TEST_BOOL(it.FindLastSubString_NoCase("def") == sz + 3);
    PLASMA_TEST_BOOL(it.FindLastSubString_NoCase("cd") == sz + 2);
    PLASMA_TEST_BOOL(it.FindLastSubString_NoCase("") == nullptr);
    PLASMA_TEST_BOOL(it.FindLastSubString_NoCase(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it.FindLastSubString_NoCase("g") == nullptr);

    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.FindLastSubString_NoCase("abcdef") == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString_NoCase("abc") == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString_NoCase("de") == sz + 3);
    PLASMA_TEST_BOOL(it2.FindLastSubString_NoCase("cd") == sz + 2);
    PLASMA_TEST_BOOL(it2.FindLastSubString_NoCase("") == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString_NoCase(nullptr) == nullptr);
    PLASMA_TEST_BOOL(it2.FindLastSubString_NoCase("g") == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compare")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.Compare("abcdef") == 0);
    PLASMA_TEST_BOOL(it.Compare("abcde") > 0);
    PLASMA_TEST_BOOL(it.Compare("abcdefg") < 0);

    plStringView it2(sz + 2, sz + 5);
    it2.SetStartPosition(sz + 3);

    PLASMA_TEST_BOOL(it2.Compare("de") == 0);
    PLASMA_TEST_BOOL(it2.Compare("def") < 0);
    PLASMA_TEST_BOOL(it2.Compare("d") > 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compare_NoCase")
  {
    const char* sz = "ABCDEF";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.Compare_NoCase("abcdef") == 0);
    PLASMA_TEST_BOOL(it.Compare_NoCase("abcde") > 0);
    PLASMA_TEST_BOOL(it.Compare_NoCase("abcdefg") < 0);

    plStringView it2(sz + 2, sz + 5);
    it2.SetStartPosition(sz + 3);

    PLASMA_TEST_BOOL(it2.Compare_NoCase("de") == 0);
    PLASMA_TEST_BOOL(it2.Compare_NoCase("def") < 0);
    PLASMA_TEST_BOOL(it2.Compare_NoCase("d") > 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareN")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.CompareN("abc", 3) == 0);
    PLASMA_TEST_BOOL(it.CompareN("abcde", 6) > 0);
    PLASMA_TEST_BOOL(it.CompareN("abcg", 3) == 0);

    plStringView it2(sz + 2, sz + 5);

    PLASMA_TEST_BOOL(it2.CompareN("cd", 2) == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareN_NoCase")
  {
    const char* sz = "ABCDEF";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.CompareN_NoCase("abc", 3) == 0);
    PLASMA_TEST_BOOL(it.CompareN_NoCase("abcde", 6) > 0);
    PLASMA_TEST_BOOL(it.CompareN_NoCase("abcg", 3) == 0);

    plStringView it2(sz + 2, sz + 5);

    PLASMA_TEST_BOOL(it2.CompareN_NoCase("cd", 2) == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.IsEqual("abcdef"));
    PLASMA_TEST_BOOL(!it.IsEqual("abcde"));
    PLASMA_TEST_BOOL(!it.IsEqual("abcdefg"));

    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.IsEqual("cde"));
    PLASMA_TEST_BOOL(!it2.IsEqual("bcde"));
    PLASMA_TEST_BOOL(!it2.IsEqual("cdef"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual_NoCase")
  {
    const char* sz = "ABCDEF";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.IsEqual_NoCase("abcdef"));
    PLASMA_TEST_BOOL(!it.IsEqual_NoCase("abcde"));
    PLASMA_TEST_BOOL(!it.IsEqual_NoCase("abcdefg"));

    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.IsEqual_NoCase("cde"));
    PLASMA_TEST_BOOL(!it2.IsEqual_NoCase("bcde"));
    PLASMA_TEST_BOOL(!it2.IsEqual_NoCase("cdef"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqualN")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.IsEqualN("abcGHI", 3));
    PLASMA_TEST_BOOL(!it.IsEqualN("abcGHI", 4));

    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.IsEqualN("cdeZX", 3));
    PLASMA_TEST_BOOL(!it2.IsEqualN("cdeZX", 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqualN_NoCase")
  {
    const char* sz = "ABCDEF";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.IsEqualN_NoCase("abcGHI", 3));
    PLASMA_TEST_BOOL(!it.IsEqualN_NoCase("abcGHI", 4));

    plStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    PLASMA_TEST_BOOL(it2.IsEqualN_NoCase("cdeZX", 3));
    PLASMA_TEST_BOOL(!it2.IsEqualN_NoCase("cdeZX", 4));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator==/!=")
  {
    const char* sz = "abcdef";
    const char* sz2 = "blabla";
    plStringView it(sz);
    plStringView it2(sz);
    plStringView it3(sz2);

    PLASMA_TEST_BOOL(it == sz);
    PLASMA_TEST_BOOL(sz == it);
    PLASMA_TEST_BOOL(it == "abcdef");
    PLASMA_TEST_BOOL("abcdef" == it);
    PLASMA_TEST_BOOL(it == it);
    PLASMA_TEST_BOOL(it == it2);

    PLASMA_TEST_BOOL(it != sz2);
    PLASMA_TEST_BOOL(sz2 != it);
    PLASMA_TEST_BOOL(it != "blabla");
    PLASMA_TEST_BOOL("blabla" != it);
    PLASMA_TEST_BOOL(it != it3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "substring operator ==/!=/</>/<=/>=")
  {

    const char* sz1 = "aaabbbcccddd";
    const char* sz2 = "aaabbbdddeee";

    plStringView it1(sz1 + 3, sz1 + 6);
    plStringView it2(sz2 + 3, sz2 + 6);

    PLASMA_TEST_BOOL(it1 == it1);
    PLASMA_TEST_BOOL(it2 == it2);

    PLASMA_TEST_BOOL(it1 == it2);
    PLASMA_TEST_BOOL(!(it1 != it2));
    PLASMA_TEST_BOOL(!(it1 < it2));
    PLASMA_TEST_BOOL(!(it1 > it2));
    PLASMA_TEST_BOOL(it1 <= it2);
    PLASMA_TEST_BOOL(it1 >= it2);

    it1 = plStringView(sz1 + 3, sz1 + 7);
    it2 = plStringView(sz2 + 3, sz2 + 7);

    PLASMA_TEST_BOOL(it1 == it1);
    PLASMA_TEST_BOOL(it2 == it2);

    PLASMA_TEST_BOOL(it1 != it2);
    PLASMA_TEST_BOOL(!(it1 == it2));

    PLASMA_TEST_BOOL(it1 < it2);
    PLASMA_TEST_BOOL(!(it1 > it2));
    PLASMA_TEST_BOOL(it1 <= it2);
    PLASMA_TEST_BOOL(!(it1 >= it2));

    PLASMA_TEST_BOOL(it2 > it1);
    PLASMA_TEST_BOOL(!(it2 < it1));
    PLASMA_TEST_BOOL(it2 >= it1);
    PLASMA_TEST_BOOL(!(it2 <= it1));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator</>")
  {
    const char* sz = "abcdef";
    const char* sz2 = "abcdefg";
    plStringView it(sz);
    plStringView it2(sz2);

    PLASMA_TEST_BOOL(it < sz2);
    PLASMA_TEST_BOOL(sz < it2);
    PLASMA_TEST_BOOL(it < it2);

    PLASMA_TEST_BOOL(sz2 > it);
    PLASMA_TEST_BOOL(it2 > sz);
    PLASMA_TEST_BOOL(it2 > it);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator<=/>=")
  {
    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdefg";
      plStringView it(sz);
      plStringView it2(sz2);

      PLASMA_TEST_BOOL(it <= sz2);
      PLASMA_TEST_BOOL(sz <= it2);
      PLASMA_TEST_BOOL(it <= it2);

      PLASMA_TEST_BOOL(sz2 >= it);
      PLASMA_TEST_BOOL(it2 >= sz);
      PLASMA_TEST_BOOL(it2 >= it);
    }

    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdef";
      plStringView it(sz);
      plStringView it2(sz2);

      PLASMA_TEST_BOOL(it <= sz2);
      PLASMA_TEST_BOOL(sz <= it2);
      PLASMA_TEST_BOOL(it <= it2);

      PLASMA_TEST_BOOL(sz2 >= it);
      PLASMA_TEST_BOOL(it2 >= sz);
      PLASMA_TEST_BOOL(it2 >= it);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindWholeWord")
  {
    plStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    plStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    plStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    PLASMA_TEST_BOOL(it.FindWholeWord("abc", plStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[34]);
    PLASMA_TEST_BOOL(it.FindWholeWord("def", plStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[38]);
    PLASMA_TEST_BOOL(
      it.FindWholeWord("mompfh", plStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[0]); // ü is not English (thus a delimiter)

    PLASMA_TEST_BOOL(it.FindWholeWord("abc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    PLASMA_TEST_BOOL(it.FindWholeWord("abc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == nullptr);

    PLASMA_TEST_BOOL(it2.FindWholeWord("abc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    PLASMA_TEST_BOOL(it2.FindWholeWord("abc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == &it.GetStartPointer()[42]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    plStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    plStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    plStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    PLASMA_TEST_BOOL(it.FindWholeWord_NoCase("ABC", plStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[34]);
    PLASMA_TEST_BOOL(it.FindWholeWord_NoCase("DEF", plStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[38]);
    PLASMA_TEST_BOOL(it.FindWholeWord_NoCase("momPFH", plStringUtils::IsWordDelimiter_English) == &it.GetStartPointer()[0]);

    PLASMA_TEST_BOOL(it.FindWholeWord_NoCase("ABc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    PLASMA_TEST_BOOL(it.FindWholeWord_NoCase("ABc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == nullptr);

    PLASMA_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 34) == &it.GetStartPointer()[34]);
    PLASMA_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", plStringUtils::IsWordDelimiter_English, it.GetStartPointer() + 35) == &it.GetStartPointer()[42]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ComputeCharacterPosition")
  {
    const wchar_t* sz = L"mompfhüßß ßßß öäü abcdef abc def abc def";
    plStringBuilder s(sz);

    PLASMA_TEST_STRING(s.ComputeCharacterPosition(14), plStringUtf8(L"öäü abcdef abc def abc def").GetData());
  }
}
