#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/String.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Strings);

PLASMA_CREATE_SIMPLE_TEST(Strings, StringUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsNullOrEmpty")
  {
    PLASMA_TEST_BOOL(plStringUtils::IsNullOrEmpty((char*)nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsNullOrEmpty("") == true);

    // all other characters are not empty
    for (plUInt8 c = 1; c < 255; c++)
      PLASMA_TEST_BOOL(plStringUtils::IsNullOrEmpty(&c) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetStringElementCount")
  {
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount((char*)nullptr), 0);

    // Counts the Bytes
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(""), 0);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount("a"), 1);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount("ab"), 2);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount("abc"), 3);

    // Counts the number of wchar_t's
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(L""), 0);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(L"a"), 1);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(L"ab"), 2);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(L"abc"), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(sz, sz + 0), 0);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(sz, sz + 3), 3);
    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(sz, sz + 6), 6);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "UpdateStringEnd")
  {
    const char* sz = "Test test";
    const char* szEnd = plUnicodeUtils::GetMaxStringEnd<char>();

    plStringUtils::UpdateStringEnd(sz, szEnd);
    PLASMA_TEST_BOOL(szEnd == sz + plStringUtils::GetStringElementCount(sz));

    plStringUtils::UpdateStringEnd(sz, szEnd);
    PLASMA_TEST_BOOL(szEnd == sz + plStringUtils::GetStringElementCount(sz));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCharacterCount")
  {
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(nullptr), 0);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(""), 0);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount("a"), 1);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount("abc"), 3);

    plStringUtf8 s(L"äöü"); // 6 Bytes

    PLASMA_TEST_INT(plStringUtils::GetStringElementCount(s.GetData()), 6);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(s.GetData()), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(sz, sz + 0), 0);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(sz, sz + 3), 3);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(sz, sz + 6), 6);

    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 0), 0);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 2), 1);
    PLASMA_TEST_INT(plStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 4), 2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCharacterAndElementCount")
  {
    plUInt32 uiCC, uiEC;

    plStringUtils::GetCharacterAndElementCount(nullptr, uiCC, uiEC);
    PLASMA_TEST_INT(uiCC, 0);
    PLASMA_TEST_INT(uiEC, 0);

    plStringUtils::GetCharacterAndElementCount("", uiCC, uiEC);
    PLASMA_TEST_INT(uiCC, 0);
    PLASMA_TEST_INT(uiEC, 0);

    plStringUtils::GetCharacterAndElementCount("a", uiCC, uiEC);
    PLASMA_TEST_INT(uiCC, 1);
    PLASMA_TEST_INT(uiEC, 1);

    plStringUtils::GetCharacterAndElementCount("abc", uiCC, uiEC);
    PLASMA_TEST_INT(uiCC, 3);
    PLASMA_TEST_INT(uiEC, 3);

    plStringUtf8 s(L"äöü"); // 6 Bytes

    plStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC);
    PLASMA_TEST_INT(uiCC, 3);
    PLASMA_TEST_INT(uiEC, 6);

    plStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 0);
    PLASMA_TEST_INT(uiCC, 0);
    PLASMA_TEST_INT(uiEC, 0);

    plStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 4);
    PLASMA_TEST_INT(uiCC, 2);
    PLASMA_TEST_INT(uiEC, 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy (full)")
  {
    char szDest[256] = "";

    // large enough
    PLASMA_TEST_INT(plStringUtils::Copy(szDest, 256, "Test ABC"), 8);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szDest, "Test ABC"));

    // exactly fitting
    PLASMA_TEST_INT(plStringUtils::Copy(szDest, 13, "Humpf, humpf"), 12);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szDest, "Humpf, humpf"));

    // too small
    PLASMA_TEST_INT(plStringUtils::Copy(szDest, 8, "Test ABC"), 7);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szDest, "Test AB"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    // large enough
    PLASMA_TEST_INT(plStringUtils::Copy(szDest, 256, szUTF8), 9);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szDest, szUTF8));

    // exactly fitting
    PLASMA_TEST_INT(plStringUtils::Copy(szDest, 10, szUTF8), 9);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szDest, szUTF8));

    // These tests are disabled as previously valid behavior was now turned into an assert.
    // Comment them in to test the assert.
    // too small 1
    /*PLASMA_TEST_INT(plStringUtils::Copy(szDest, 9, szUTF8), 7);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 5)); // one character less

    // too small 2
    PLASMA_TEST_INT(plStringUtils::Copy(szDest, 7, szUTF8), 4);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 4)); // two characters less*/


    // copy only from a subset
    PLASMA_TEST_INT(plStringUtils::Copy(szDest, 256, szUTF8, szUTF8 + 7), 7);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 5)); // two characters less
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CopyN")
  {
    char szDest[256] = "";

    PLASMA_TEST_INT(plStringUtils::CopyN(szDest, 256, "Test ABC", 4), 4);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szDest, "Test"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    PLASMA_TEST_INT(plStringUtils::CopyN(szDest, 256, szUTF8, 6), 9);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 6));

    PLASMA_TEST_INT(plStringUtils::CopyN(szDest, 256, szUTF8, 5), 7);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 5));

    PLASMA_TEST_INT(plStringUtils::CopyN(szDest, 256, szUTF8, 4), 4);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 4));

    PLASMA_TEST_INT(plStringUtils::CopyN(szDest, 256, szUTF8, 1), 1);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 1));

    PLASMA_TEST_INT(plStringUtils::CopyN(szDest, 256, szUTF8, 0), 0);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szDest, ""));

    // copy only from a subset
    PLASMA_TEST_INT(plStringUtils::CopyN(szDest, 256, szUTF8, 6, szUTF8 + 7), 7);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(szDest, szUTF8, 5));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ToUpperChar")
  {
    // this only tests the ASCII range
    for (plInt32 i = 0; i < 128; ++i)
      PLASMA_TEST_INT(plStringUtils::ToUpperChar(i), toupper(i));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ToLowerChar")
  {
    // this only tests the ASCII range
    for (plInt32 i = 0; i < 128; ++i)
      PLASMA_TEST_INT(plStringUtils::ToLowerChar(i), tolower(i));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ToUpperString")
  {
    plStringUtf8 sL(L"abc öäü ß €");
    plStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    plStringUtils::Copy(szCopy, 256, sL.GetData());

    plStringUtils::ToUpperString(szCopy);

    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szCopy, sU.GetData()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ToLowerString")
  {
    plStringUtf8 sL(L"abc öäü ß €");
    plStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    plStringUtils::Copy(szCopy, 256, sU.GetData());

    plStringUtils::ToLowerString(szCopy);

    PLASMA_TEST_BOOL(plStringUtils::IsEqual(szCopy, sL.GetData()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareChars")
  {
    PLASMA_TEST_BOOL(plStringUtils::CompareChars('a', 'a') == 0); // make sure the order is right
    PLASMA_TEST_BOOL(plStringUtils::CompareChars('a', 'b') < 0);  // a smaller than b -> negative
    PLASMA_TEST_BOOL(plStringUtils::CompareChars('b', 'a') > 0);  // b bigger than a  -> positive
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareChars(utf8)")
  {
    PLASMA_TEST_BOOL(plStringUtils::CompareChars("a", "a") == 0); // make sure the order is right
    PLASMA_TEST_BOOL(plStringUtils::CompareChars("a", "b") < 0);  // a smaller than b -> negative
    PLASMA_TEST_BOOL(plStringUtils::CompareChars("b", "a") > 0);  // b bigger than a  -> positive
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareChars_NoCase")
  {
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase('a', 'A') == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase('a', 'B') < 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase('B', 'a') > 0);

    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase('A', 'a') == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase('A', 'b') < 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase('b', 'A') > 0);

    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase(L'ä', L'Ä') == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase(L'ä', L'Ö') < 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase(L'ö', L'Ä') > 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareChars_NoCase(utf8)")
  {
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase("a", "A") == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase("a", "B") < 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase("B", "a") > 0);

    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase("A", "a") == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase("A", "b") < 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareChars_NoCase("b", "A") > 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(nullptr, nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(nullptr, "") == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual("", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual("", "") == true);

    PLASMA_TEST_BOOL(plStringUtils::IsEqual("abc", "abc") == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual("abc", "abcd") == false);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual("abcd", "abc") == false);

    PLASMA_TEST_BOOL(plStringUtils::IsEqual("a", nullptr) == false);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual(nullptr, "a") == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqualN")
  {
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(nullptr, nullptr, 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(nullptr, "", 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("", nullptr, 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abc", nullptr, 0) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abc", "", 0) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN(nullptr, "abc", 0) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("", "abc", 0) == true);

    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abc", "abcdef", 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abc", "abcdef", 2) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abc", "abcdef", 3) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abc", "abcdef", 4) == false);

    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abcdef", "abc", 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abcdef", "abc", 2) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abcdef", "abc", 3) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN("abcdef", "abc", 4) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual_NoCase")
  {
    PLASMA_TEST_BOOL(plStringUtils::IsEqual_NoCase(nullptr, nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual_NoCase(nullptr, "") == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual_NoCase("", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual_NoCase("", "") == true);


    plStringUtf8 sL(L"abc öäü ß €");
    plStringUtf8 sU(L"ABC ÖÄÜ ß €");
    plStringUtf8 sU2(L"ABC ÖÄÜ ß € ");

    PLASMA_TEST_BOOL(plStringUtils::IsEqual_NoCase(sL.GetData(), sU.GetData()) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual_NoCase(sL.GetData(), sU2.GetData()) == false);
    PLASMA_TEST_BOOL(plStringUtils::IsEqual_NoCase(sU2.GetData(), sL.GetData()) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqualN_NoCase")
  {
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase(nullptr, nullptr, 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase(nullptr, "", 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase("", nullptr, 1) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase("abc", nullptr, 0) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase("abc", "", 0) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase(nullptr, "abc", 0) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase("", "abc", 0) == true);

    plStringUtf8 sL(L"abc öäü ß €");
    plStringUtf8 sU(L"ABC ÖÄÜ ß € moep");

    for (plInt32 i = 0; i < 12; ++i)
      PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), i) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), 12) == false);

    for (plInt32 i = 0; i < 12; ++i)
      PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), i) == true);
    PLASMA_TEST_BOOL(plStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), 12) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compare")
  {
    PLASMA_TEST_BOOL(plStringUtils::Compare(nullptr, nullptr) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare(nullptr, "") == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare("", nullptr) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare("", "") == 0);

    PLASMA_TEST_BOOL(plStringUtils::Compare("abc", "abc") == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare("abc", "abcd") < 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare("abcd", "abc") > 0);

    PLASMA_TEST_BOOL(plStringUtils::Compare("a", nullptr) > 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    PLASMA_TEST_BOOL(plStringUtils::Compare(sz, "abc", sz + 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare(sz, "abc def", sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare(sz, sz, sz + 7, sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare(sz, sz, sz + 7, sz + 6) > 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare(sz, sz, sz + 7, sz + 8) < 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareN")
  {
    PLASMA_TEST_BOOL(plStringUtils::CompareN(nullptr, nullptr, 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN(nullptr, "", 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("", nullptr, 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abc", nullptr, 0) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abc", "", 0) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN(nullptr, "abc", 0) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("", "abc", 0) == 0);

    PLASMA_TEST_BOOL(plStringUtils::CompareN("abc", "abcdef", 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abc", "abcdef", 2) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abc", "abcdef", 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abc", "abcdef", 4) < 0);

    PLASMA_TEST_BOOL(plStringUtils::CompareN("abcdef", "abc", 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abcdef", "abc", 2) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abcdef", "abc", 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN("abcdef", "abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    PLASMA_TEST_BOOL(plStringUtils::CompareN(sz, "abc", 10, sz + 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN(sz, "abc def", 10, sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 6) > 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compare_NoCase")
  {
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(nullptr, nullptr) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(nullptr, "") == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase("", nullptr) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase("", "") == 0);

    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase("abc", "aBc") == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase("ABC", "abcd") < 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase("abcd", "ABC") > 0);

    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase("a", nullptr) > 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(sz, "ABC", sz + 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(sz, "ABC def", sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 6) > 0);
    PLASMA_TEST_BOOL(plStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 8) < 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompareN_NoCase")
  {
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(nullptr, nullptr, 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(nullptr, "", 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("", nullptr, 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("abc", nullptr, 0) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("abc", "", 0) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(nullptr, "abc", 0) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("", "abc", 0) == 0);

    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("aBc", "abcdef", 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("aBc", "abcdef", 2) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("aBc", "abcdef", 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("aBc", "abcdef", 4) < 0);

    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("abcdef", "Abc", 1) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("abcdef", "Abc", 2) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("abcdef", "Abc", 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase("abcdef", "Abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(sz, "ABC", 10, sz + 3) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(sz, "ABC def", 10, sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 7) == 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 6) > 0);
    PLASMA_TEST_BOOL(plStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "snprintf")
  {
    // This function has been tested to death during its implementation.
    // That test-code would require several pages, if one would try to test it properly.
    // I am not going to do that here, I am quite confident the function works as expected with pure ASCII strings.
    // So I'm only testing a bit of Utf8 stuff.

    plStringUtf8 s(L"Abc %s äöü ß %i %s %.4f");
    plStringUtf8 s2(L"ÄÖÜ");

    char sz[256];
    plStringUtils::snprintf(sz, 256, s.GetData(), "ASCII", 42, s2.GetData(), 23.31415);

    plStringUtf8 sC(L"Abc ASCII äöü ß 42 ÄÖÜ 23.3142"); // notice the correct float rounding ;-)

    PLASMA_TEST_STRING(sz, sC.GetData());


    // NaN and Infinity
    plStringUtils::snprintf(sz, 256, "NaN Value: %.2f", plMath::NaN<float>());
    PLASMA_TEST_STRING(sz, "NaN Value: NaN");

    plStringUtils::snprintf(sz, 256, "Inf Value: %.2f", +plMath::Infinity<float>());
    PLASMA_TEST_STRING(sz, "Inf Value: Infinity");

    plStringUtils::snprintf(sz, 256, "Inf Value: %.2f", -plMath::Infinity<float>());
    PLASMA_TEST_STRING(sz, "Inf Value: -Infinity");

    plStringUtils::snprintf(sz, 256, "NaN Value: %.2e", plMath::NaN<float>());
    PLASMA_TEST_STRING(sz, "NaN Value: NaN");

    plStringUtils::snprintf(sz, 256, "Inf Value: %.2e", +plMath::Infinity<float>());
    PLASMA_TEST_STRING(sz, "Inf Value: Infinity");

    plStringUtils::snprintf(sz, 256, "Inf Value: %.2e", -plMath::Infinity<float>());
    PLASMA_TEST_STRING(sz, "Inf Value: -Infinity");

    plStringUtils::snprintf(sz, 256, "NaN Value: %+10.2f", plMath::NaN<float>());
    PLASMA_TEST_STRING(sz, "NaN Value:       +NaN");

    plStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", +plMath::Infinity<float>());
    PLASMA_TEST_STRING(sz, "Inf Value:  +Infinity");

    plStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", -plMath::Infinity<float>());
    PLASMA_TEST_STRING(sz, "Inf Value:  -Infinity");

    // extended stuff
    plStringUtils::snprintf(sz, 256, "size: %zu", (size_t)12345678);
    PLASMA_TEST_STRING(sz, "size: 12345678");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StartsWith")
  {
    PLASMA_TEST_BOOL(plStringUtils::StartsWith(nullptr, nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith(nullptr, "") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith("", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith("", "") == true);

    PLASMA_TEST_BOOL(plStringUtils::StartsWith("abc", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith("abc", "") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith(nullptr, "abc") == false);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith("", "abc") == false);

    PLASMA_TEST_BOOL(plStringUtils::StartsWith("abc", "abc") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith("abcdef", "abc") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith("abcdef", "Abc") == false);

    // substring test
    const char* sz = u8"äbc def ghi";
    const plUInt32 uiByteCount = plStringUtils::GetStringElementCount(u8"äbc");

    PLASMA_TEST_BOOL(plStringUtils::StartsWith(sz, u8"äbc", sz + uiByteCount) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith(sz, u8"äbc", sz + uiByteCount - 1) == false);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith(sz, u8"äbc", sz + 0) == false);

    const char* sz2 = u8"äbc def";
    PLASMA_TEST_BOOL(plStringUtils::StartsWith(sz, sz2, sz + uiByteCount, sz2 + uiByteCount) == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StartsWith_NoCase")
  {
    plStringUtf8 sL(L"äöü");
    plStringUtf8 sU(L"ÄÖÜ");

    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(nullptr, nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(nullptr, "") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("", "") == true);

    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("abc", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("abc", "") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(nullptr, "abc") == false);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("", "abc") == false);

    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("abc", "ABC") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("aBCdef", "abc") == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase("aBCdef", "bc") == false);

    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char* sz = u8"äbc def ghi";
    const plUInt32 uiByteCount = plStringUtils::GetStringElementCount(u8"äbc");
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(sz, u8"ÄBC", sz + uiByteCount) == true);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(sz, u8"ÄBC", sz + uiByteCount - 1) == false);
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(sz, u8"ÄBC", sz + 0) == false);

    const char* sz2 = u8"Äbc def";
    PLASMA_TEST_BOOL(plStringUtils::StartsWith_NoCase(sz, sz2, sz + uiByteCount, sz2 + uiByteCount) == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EndsWith")
  {
    PLASMA_TEST_BOOL(plStringUtils::EndsWith(nullptr, nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith(nullptr, "") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith("", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith("", "") == true);

    PLASMA_TEST_BOOL(plStringUtils::EndsWith("abc", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith("abc", "") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith(nullptr, "abc") == false);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith("", "abc") == false);

    PLASMA_TEST_BOOL(plStringUtils::EndsWith("abc", "abc") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith("abcdef", "def") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith("abcdef", "Def") == false);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith("def", "abcdef") == false);

    // substring test
    const char* sz = "abc def ghi";
    PLASMA_TEST_BOOL(plStringUtils::EndsWith(sz, "abc", sz + 3) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith(sz, "def", sz + 7) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith(sz, "def", sz + 8) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EndsWith_NoCase")
  {
    plStringUtf8 sL(L"äöü");
    plStringUtf8 sU(L"ÄÖÜ");

    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase(nullptr, nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase(nullptr, "") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("", "") == true);

    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("abc", nullptr) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("abc", "") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase(nullptr, "abc") == false);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("", "abc") == false);

    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("abc", "abc") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("abcdef", "def") == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("abcdef", "Def") == true);

    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase("def", "abcdef") == false);

    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char* sz = "abc def ghi";
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase(sz, "ABC", sz + 3) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase(sz, "DEF", sz + 7) == true);
    PLASMA_TEST_BOOL(plStringUtils::EndsWith_NoCase(sz, "DEF", sz + 8) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindSubString")
  {
    plStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    plStringUtf8 s2(L"äöü");
    plStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    PLASMA_TEST_BOOL(plStringUtils::FindSubString(szABC, szABC) == szABC);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString("abc", "") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString("abc", nullptr) == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(nullptr, "abc") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString("", "abc") == nullptr);

    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "abc") == s.GetData());
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "def") == &s.GetData()[4]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "ghi") == &s.GetData()[8]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "abc2") == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "def2") == &s.GetData()[35]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "ghi2") == &s.GetData()[40]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 34) == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 33) == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindSubString_NoCase")
  {
    plStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    plStringUtf8 s2(L"äÖü");
    plStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(szABC, "aBc") == szABC);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase("abc", "") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase("abc", nullptr) == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(nullptr, "abc") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase("", "abc") == nullptr);

    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "Abc") == s.GetData());
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[4]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[8]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "abC2") == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "dEf2") == &s.GetData()[35]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "Ghi2") == &s.GetData()[40]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "aBc2", s.GetData() + 34) == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindSubString_NoCase(s.GetData(), "abC2", s.GetData() + 33) == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindLastSubString")
  {
    plStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    plStringUtf8 s2(L"äöü");
    plStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(szABC, szABC) == szABC);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString("abc", "") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString("abc", nullptr) == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(nullptr, "abc") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString("", "abc") == nullptr);

    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(s.GetData(), "abc") == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(s.GetData(), "def") == &s.GetData()[35]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(s.GetData(), "ghi") == &s.GetData()[40]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    plStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    plStringUtf8 s2(L"äÖü");
    plStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(szABC, "aBC") == szABC);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase("abc", "") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase("abc", nullptr) == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(nullptr, "abc") == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase("", "abc") == nullptr);

    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(s.GetData(), "Abc") == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[35]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[40]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    PLASMA_TEST_BOOL(plStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindWholeWord")
  {
    plStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord(s.GetData(), "abc", plStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord(s.GetData(), "def", plStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord(s.GetData(), "mompfh", plStringUtils::IsWordDelimiter_English) == &s.GetData()[0]); // ü is not english

    // substring test
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord(s.GetData(), "abc", plStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord(s.GetData(), "abc", plStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord(s.GetData(), "abc", plStringUtils::IsWordDelimiter_English, s.GetData() + 30) == s.GetData() + 27);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    plStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", plStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord_NoCase(s.GetData(), "DEF", plStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord_NoCase(s.GetData(), "momPFH", plStringUtils::IsWordDelimiter_English) == &s.GetData()[0]);

    // substring test
    PLASMA_TEST_BOOL(
      plStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", plStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    PLASMA_TEST_BOOL(plStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", plStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindUIntAtTheEnd")
  {
    plUInt32 uiTestValue = 0;
    plUInt32 uiCharactersFromStart = 0;

    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(nullptr, uiTestValue, &uiCharactersFromStart).Failed());

    plStringUtf8 noNumberAtTheEnd(L"ThisStringContainsNoNumberAtTheEnd");
    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(noNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Failed());

    plStringUtf8 noNumberAtTheEnd2(L"ThisStringContainsNoNumberAtTheEndBut42InBetween");
    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(noNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Failed());

    plStringUtf8 aNumberAtTheEnd(L"ThisStringContainsANumberAtTheEnd1");
    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(aNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    PLASMA_TEST_INT(uiTestValue, 1);
    PLASMA_TEST_INT(uiCharactersFromStart, aNumberAtTheEnd.GetElementCount() - 1);

    plStringUtf8 aZeroLeadingNumberAtTheEnd(L"ThisStringContainsANumberAtTheEnd011129");
    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(aZeroLeadingNumberAtTheEnd.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    PLASMA_TEST_INT(uiTestValue, 11129);
    PLASMA_TEST_INT(uiCharactersFromStart, aZeroLeadingNumberAtTheEnd.GetElementCount() - 6);

    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(aNumberAtTheEnd.GetData(), uiTestValue, nullptr).Succeeded());
    PLASMA_TEST_INT(uiTestValue, 1);

    plStringUtf8 twoNumbersInOneString(L"FirstANumber23AndThen42");
    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(twoNumbersInOneString.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    PLASMA_TEST_INT(uiTestValue, 42);

    plStringUtf8 onlyANumber(L"55566553");
    PLASMA_TEST_BOOL(plStringUtils::FindUIntAtTheEnd(onlyANumber.GetData(), uiTestValue, &uiCharactersFromStart).Succeeded());
    PLASMA_TEST_INT(uiTestValue, 55566553);
    PLASMA_TEST_INT(uiCharactersFromStart, 0);
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SkipCharacters")
  {
    plStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char* szEmpty = "";

    PLASMA_TEST_BOOL(plStringUtils::SkipCharacters(s.GetData(), plStringUtils::IsWhiteSpace, false) == &s.GetData()[0]);
    PLASMA_TEST_BOOL(plStringUtils::SkipCharacters(s.GetData(), plStringUtils::IsWhiteSpace, true) == &s.GetData()[1]);
    PLASMA_TEST_BOOL(plStringUtils::SkipCharacters(&s.GetData()[5], plStringUtils::IsWhiteSpace, false) == &s.GetData()[8]);
    PLASMA_TEST_BOOL(plStringUtils::SkipCharacters(&s.GetData()[5], plStringUtils::IsWhiteSpace, true) == &s.GetData()[8]);
    PLASMA_TEST_BOOL(plStringUtils::SkipCharacters(szEmpty, plStringUtils::IsWhiteSpace, false) == szEmpty);
    PLASMA_TEST_BOOL(plStringUtils::SkipCharacters(szEmpty, plStringUtils::IsWhiteSpace, true) == szEmpty);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindWordEnd")
  {
    plStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char* szEmpty = "";

    PLASMA_TEST_BOOL(plStringUtils::FindWordEnd(s.GetData(), plStringUtils::IsWhiteSpace, true) == &s.GetData()[5]);
    PLASMA_TEST_BOOL(plStringUtils::FindWordEnd(s.GetData(), plStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    PLASMA_TEST_BOOL(plStringUtils::FindWordEnd(&s.GetData()[5], plStringUtils::IsWhiteSpace, true) == &s.GetData()[6]);
    PLASMA_TEST_BOOL(plStringUtils::FindWordEnd(&s.GetData()[5], plStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    PLASMA_TEST_BOOL(plStringUtils::FindWordEnd(szEmpty, plStringUtils::IsWhiteSpace, true) == szEmpty);
    PLASMA_TEST_BOOL(plStringUtils::FindWordEnd(szEmpty, plStringUtils::IsWhiteSpace, false) == szEmpty);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsWhitespace")
  {
    PLASMA_TEST_BOOL(plStringUtils::IsWhiteSpace(' '));
    PLASMA_TEST_BOOL(plStringUtils::IsWhiteSpace('\t'));
    PLASMA_TEST_BOOL(plStringUtils::IsWhiteSpace('\n'));
    PLASMA_TEST_BOOL(plStringUtils::IsWhiteSpace('\r'));
    PLASMA_TEST_BOOL(plStringUtils::IsWhiteSpace('\v'));

    PLASMA_TEST_BOOL(plStringUtils::IsWhiteSpace('\0') == false);

    for (plUInt32 i = 33; i < 256; ++i)
    {
      PLASMA_TEST_BOOL(plStringUtils::IsWhiteSpace(i) == false);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsDecimalDigit / IsHexDigit")
  {
    PLASMA_TEST_BOOL(plStringUtils::IsDecimalDigit('0'));
    PLASMA_TEST_BOOL(plStringUtils::IsDecimalDigit('4'));
    PLASMA_TEST_BOOL(plStringUtils::IsDecimalDigit('9'));
    PLASMA_TEST_BOOL(!plStringUtils::IsDecimalDigit('/'));
    PLASMA_TEST_BOOL(!plStringUtils::IsDecimalDigit('A'));

    PLASMA_TEST_BOOL(plStringUtils::IsHexDigit('0'));
    PLASMA_TEST_BOOL(plStringUtils::IsHexDigit('4'));
    PLASMA_TEST_BOOL(plStringUtils::IsHexDigit('9'));
    PLASMA_TEST_BOOL(plStringUtils::IsHexDigit('A'));
    PLASMA_TEST_BOOL(plStringUtils::IsHexDigit('E'));
    PLASMA_TEST_BOOL(plStringUtils::IsHexDigit('a'));
    PLASMA_TEST_BOOL(plStringUtils::IsHexDigit('f'));
    PLASMA_TEST_BOOL(!plStringUtils::IsHexDigit('g'));
    PLASMA_TEST_BOOL(!plStringUtils::IsHexDigit('/'));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsWordDelimiter_English / IsIdentifierDelimiter_C_Code")
  {
    for (plUInt32 i = 0; i < 256; ++i)
    {
      const bool alpha = (i >= 'a' && i <= 'z');
      const bool alpha2 = (i >= 'A' && i <= 'Z');
      const bool num = (i >= '0' && i <= '9');
      const bool dash = i == '-';
      const bool underscore = i == '_';

      const bool bCode = alpha || alpha2 || num || underscore;
      const bool bWord = bCode || dash;


      PLASMA_TEST_BOOL(plStringUtils::IsWordDelimiter_English(i) == !bWord);
      PLASMA_TEST_BOOL(plStringUtils::IsIdentifierDelimiter_C_Code(i) == !bCode);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsValidIdentifierName")
  {
    PLASMA_TEST_BOOL(!plStringUtils::IsValidIdentifierName(""));
    PLASMA_TEST_BOOL(!plStringUtils::IsValidIdentifierName("1asdf"));
    PLASMA_TEST_BOOL(!plStringUtils::IsValidIdentifierName("as df"));
    PLASMA_TEST_BOOL(!plStringUtils::IsValidIdentifierName("asdf!"));

    PLASMA_TEST_BOOL(plStringUtils::IsValidIdentifierName("asdf1"));
    PLASMA_TEST_BOOL(plStringUtils::IsValidIdentifierName("_asdf"));
  }
}
