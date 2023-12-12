#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/String.h>

static plString GetString(const char* szSz)
{
  plString s;
  s = szSz;
  return s;
}

static plStringBuilder GetStringBuilder(const char* szSz)
{
  plStringBuilder s;

  for (plUInt32 i = 0; i < 10; ++i)
    s.Append(szSz);

  return s;
}

PLASMA_CREATE_SIMPLE_TEST(Strings, String)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plString s1;
    PLASMA_TEST_BOOL(s1 == "");

    plString s2("abc");
    PLASMA_TEST_BOOL(s2 == "abc");

    plString s3(s2);
    PLASMA_TEST_BOOL(s2 == s3);
    PLASMA_TEST_BOOL(s3 == "abc");

    plString s4(L"abc");
    PLASMA_TEST_BOOL(s4 == "abc");

    plStringView it = s4.GetFirst(2);
    plString s5(it);
    PLASMA_TEST_BOOL(s5 == "ab");

    plStringBuilder strB("wobwob");
    plString s6(strB);
    PLASMA_TEST_BOOL(s6 == "wobwob");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=")
  {
    plString s2;
    s2 = "abc";
    PLASMA_TEST_BOOL(s2 == "abc");

    plString s3;
    s3 = s2;
    PLASMA_TEST_BOOL(s2 == s3);
    PLASMA_TEST_BOOL(s3 == "abc");

    plString s4;
    s4 = L"abc";
    PLASMA_TEST_BOOL(s4 == "abc");

    plString s5(L"abcdefghijklm");
    plStringView it(s5.GetData() + 2, s5.GetData() + 10);
    plString s5b = it;
    PLASMA_TEST_STRING(s5b, "cdefghij");

    plString s6(L"aölsdföasld");
    plStringBuilder strB("wobwob");
    s6 = strB;
    PLASMA_TEST_BOOL(s6 == "wobwob");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "convert to plStringView")
  {
    plString s(L"aölsdföasld");
    plStringBuilder tmp;

    plStringView sv = s;

    PLASMA_TEST_STRING(sv.GetData(tmp), plStringUtf8(L"aölsdföasld").GetData());
    PLASMA_TEST_BOOL(sv == plStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    PLASMA_TEST_STRING(sv.GetStartPointer(), "abcdef");
    PLASMA_TEST_BOOL(sv == "abcdef");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move constructor / operator")
  {
    plString s1(GetString("move me"));
    PLASMA_TEST_STRING(s1.GetData(), "move me");

    s1 = GetString("move move move move move move move move ");
    PLASMA_TEST_STRING(s1.GetData(), "move move move move move move move move ");

    plString s2(GetString("move move move move move move move move "));
    PLASMA_TEST_STRING(s2.GetData(), "move move move move move move move move ");

    s2 = GetString("move me");
    PLASMA_TEST_STRING(s2.GetData(), "move me");

    s1 = s2;
    PLASMA_TEST_STRING(s1.GetData(), "move me");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move constructor / operator (StringBuilder)")
  {
    const plString s1(GetStringBuilder("move me"));
    const plString s2(GetStringBuilder("move move move move move move move move "));

    plString s3(GetStringBuilder("move me"));
    PLASMA_TEST_BOOL(s3 == s1);

    s3 = GetStringBuilder("move move move move move move move move ");
    PLASMA_TEST_BOOL(s3 == s2);

    plString s4(GetStringBuilder("move move move move move move move move "));
    PLASMA_TEST_BOOL(s4 == s2);

    s4 = GetStringBuilder("move me");
    PLASMA_TEST_BOOL(s4 == s1);

    s3 = s4;
    PLASMA_TEST_BOOL(s3 == s1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    plString s("abcdef");
    PLASMA_TEST_BOOL(s == "abcdef");

    s.Clear();
    PLASMA_TEST_BOOL(s.IsEmpty());
    PLASMA_TEST_BOOL(s == "");
    PLASMA_TEST_BOOL(s == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetData")
  {
    const char* sz = "abcdef";

    plString s(sz);
    PLASMA_TEST_BOOL(s.GetData() != sz); // it should NOT be the exact same string
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetElementCount / GetCharacterCount")
  {
    plString s(L"abcäöü€");

    PLASMA_TEST_INT(s.GetElementCount(), 12);
    PLASMA_TEST_INT(s.GetCharacterCount(), 7);

    s = "testtest";
    PLASMA_TEST_INT(s.GetElementCount(), 8);
    PLASMA_TEST_INT(s.GetCharacterCount(), 8);

    s.Clear();

    PLASMA_TEST_INT(s.GetElementCount(), 0);
    PLASMA_TEST_INT(s.GetCharacterCount(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Convert to plStringView")
  {
    plString s(L"abcäöü€def");

    plStringView view = s;
    PLASMA_TEST_BOOL(view.StartsWith("abc"));
    PLASMA_TEST_BOOL(view.EndsWith("def"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetSubString")
  {
    plString s(L"abcäöü€def");
    plStringUtf8 s8(L"äöü€");

    plStringView it = s.GetSubString(3, 4);
    PLASMA_TEST_BOOL(it == s8.GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFirst")
  {
    plString s(L"abcäöü€def");

    PLASMA_TEST_BOOL(s.GetFirst(3) == "abc");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLast")
  {
    plString s(L"abcäöü€def");

    PLASMA_TEST_BOOL(s.GetLast(3) == "def");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadAll")
  {
    plDefaultMemoryStreamStorage StreamStorage;

    plMemoryStreamWriter MemoryWriter(&StreamStorage);
    plMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText =
      "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, plStringUtils::GetStringElementCount(szText)).IgnoreResult();

    plString s;
    s.ReadAll(MemoryReader);

    PLASMA_TEST_BOOL(s == szText);
  }
}
