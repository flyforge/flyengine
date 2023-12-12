#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

PLASMA_CREATE_SIMPLE_TEST(Strings, StringView)
{
  plStringBuilder tmp;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (simple)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    plStringView it(sz);

    PLASMA_TEST_BOOL(it.GetStartPointer() == sz);
    PLASMA_TEST_STRING(it.GetData(tmp), sz);
    PLASMA_TEST_BOOL(it.GetEndPointer() == sz + 26);
    PLASMA_TEST_INT(it.GetElementCount(), 26);

    plStringView it2(sz + 15);

    PLASMA_TEST_BOOL(it2.GetStartPointer() == &sz[15]);
    PLASMA_TEST_STRING(it2.GetData(tmp), &sz[15]);
    PLASMA_TEST_BOOL(it2.GetEndPointer() == sz + 26);
    PLASMA_TEST_INT(it2.GetElementCount(), 11);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor (complex, YARLY!)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    plStringView it(sz + 3, sz + 17);
    it.SetStartPosition(sz + 5);

    PLASMA_TEST_BOOL(it.GetStartPointer() == sz + 5);
    PLASMA_TEST_STRING(it.GetData(tmp), "fghijklmnopq");
    PLASMA_TEST_BOOL(it.GetEndPointer() == sz + 17);
    PLASMA_TEST_INT(it.GetElementCount(), 12);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor constexpr")
  {
    constexpr plStringView b = plStringView("Hello World", 10);
    PLASMA_TEST_INT(b.GetElementCount(), 10);
    PLASMA_TEST_STRING(b.GetData(tmp), "Hello Worl");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "String literal")
  {
    constexpr plStringView a = "Hello World"_plsv;
    PLASMA_TEST_INT(a.GetElementCount(), 11);
    PLASMA_TEST_STRING(a.GetData(tmp), "Hello World");

    plStringView b = "Hello Worl"_plsv;
    PLASMA_TEST_INT(b.GetElementCount(), 10);
    PLASMA_TEST_STRING(b.GetData(tmp), "Hello Worl");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator++")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    plStringView it(sz);

    for (plInt32 i = 0; i < 26; ++i)
    {
      PLASMA_TEST_INT(it.GetCharacter(), sz[i]);
      PLASMA_TEST_BOOL(it.IsValid());
      it.Shrink(1, 0);
    }

    PLASMA_TEST_BOOL(!it.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator== / operator!=")
  {
    plString s1(L"abcdefghiäöüß€");
    plString s2(L"ghiäöüß€abdef");

    plStringView it1 = s1.GetSubString(8, 4);
    plStringView it2 = s2.GetSubString(2, 4);
    plStringView it3 = s2.GetSubString(2, 5);

    PLASMA_TEST_BOOL(it1 == it2);
    PLASMA_TEST_BOOL(it1 != it3);

    PLASMA_TEST_BOOL(it1 == plString(L"iäöü").GetData());
    PLASMA_TEST_BOOL(it2 == plString(L"iäöü").GetData());
    PLASMA_TEST_BOOL(it3 == plString(L"iäöüß").GetData());

    s1 = "abcdefghijkl";
    s2 = "oghijklm";

    it1 = s1.GetSubString(6, 4);
    it2 = s2.GetSubString(1, 4);
    it3 = s2.GetSubString(1, 5);

    PLASMA_TEST_BOOL(it1 == it2);
    PLASMA_TEST_BOOL(it1 != it3);

    PLASMA_TEST_BOOL(it1 == "ghij");
    PLASMA_TEST_BOOL(it1 != "ghijk");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEqual")
  {
    const char* sz = "abcdef";
    plStringView it(sz);

    PLASMA_TEST_BOOL(it.IsEqual(plStringView("abcdef")));
    PLASMA_TEST_BOOL(!it.IsEqual(plStringView("abcde")));
    PLASMA_TEST_BOOL(!it.IsEqual(plStringView("abcdefg")));

    plStringView it2(sz + 2, sz + 5);

    const char* szRhs = "Abcdef";
    plStringView it3(szRhs + 2, szRhs + 5);
    PLASMA_TEST_BOOL(it2.IsEqual(it3));
    it3 = plStringView(szRhs + 1, szRhs + 5);
    PLASMA_TEST_BOOL(!it2.IsEqual(it3));
    it3 = plStringView(szRhs + 2, szRhs + 6);
    PLASMA_TEST_BOOL(!it2.IsEqual(it3));
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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator+=")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    plStringView it(sz);

    for (plInt32 i = 0; i < 26; i += 2)
    {
      PLASMA_TEST_INT(it.GetCharacter(), sz[i]);
      PLASMA_TEST_BOOL(it.IsValid());
      it.Shrink(2, 0);
    }

    PLASMA_TEST_BOOL(!it.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCharacter")
  {
    plStringUtf8 s(L"abcäöü€");
    plStringView it = plStringView(s.GetData());

    PLASMA_TEST_INT(it.GetCharacter(), plUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[0]));
    it.Shrink(1, 0);
    PLASMA_TEST_INT(it.GetCharacter(), plUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[1]));
    it.Shrink(1, 0);
    PLASMA_TEST_INT(it.GetCharacter(), plUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[2]));
    it.Shrink(1, 0);
    PLASMA_TEST_INT(it.GetCharacter(), plUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[3]));
    it.Shrink(1, 0);
    PLASMA_TEST_INT(it.GetCharacter(), plUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[5]));
    it.Shrink(1, 0);
    PLASMA_TEST_INT(it.GetCharacter(), plUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[7]));
    it.Shrink(1, 0);
    PLASMA_TEST_INT(it.GetCharacter(), plUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[9]));
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(!it.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetElementCount")
  {
    plStringUtf8 s(L"abcäöü€");
    plStringView it = plStringView(s.GetData());

    PLASMA_TEST_INT(it.GetElementCount(), 12);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(it.GetElementCount(), 11);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(it.GetElementCount(), 10);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(it.GetElementCount(), 9);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(it.GetElementCount(), 7);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(it.GetElementCount(), 5);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(it.IsValid());
    PLASMA_TEST_INT(it.GetElementCount(), 3);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(!it.IsValid());
    PLASMA_TEST_INT(it.GetElementCount(), 0);
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(!it.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetStartPosition")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    plStringView it(sz);

    for (plInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      PLASMA_TEST_BOOL(it.IsValid());
      PLASMA_TEST_BOOL(it.StartsWith(&sz[i]));
    }

    PLASMA_TEST_BOOL(it.IsValid());
    it.Shrink(1, 0);
    PLASMA_TEST_BOOL(!it.IsValid());

    it = plStringView(sz);
    for (plInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      PLASMA_TEST_BOOL(it.IsValid());
      PLASMA_TEST_BOOL(it.StartsWith(&sz[i]));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetStartPosition / GetEndPosition / GetData")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    plStringView it(sz + 7, sz + 19);

    PLASMA_TEST_BOOL(it.GetStartPointer() == sz + 7);
    PLASMA_TEST_BOOL(it.GetEndPointer() == sz + 19);
    PLASMA_TEST_STRING(it.GetData(tmp), "hijklmnopqrs");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Shrink")
  {
    plStringUtf8 s(L"abcäöü€def");
    plStringView it(s.GetData());

    PLASMA_TEST_BOOL(it.GetStartPointer() == &s.GetData()[0]);
    PLASMA_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    PLASMA_TEST_STRING(it.GetData(tmp), &s.GetData()[0]);
    PLASMA_TEST_BOOL(it.IsValid());

    it.Shrink(1, 0);

    PLASMA_TEST_BOOL(it.GetStartPointer() == &s.GetData()[1]);
    PLASMA_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    PLASMA_TEST_STRING(it.GetData(tmp), &s.GetData()[1]);
    PLASMA_TEST_BOOL(it.IsValid());

    it.Shrink(3, 0);

    PLASMA_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    PLASMA_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    PLASMA_TEST_STRING(it.GetData(tmp), &s.GetData()[5]);
    PLASMA_TEST_BOOL(it.IsValid());

    it.Shrink(0, 4);

    PLASMA_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    PLASMA_TEST_BOOL(it.GetEndPointer() == &s.GetData()[9]);
    PLASMA_TEST_STRING(it.GetData(tmp), u8"öü");
    PLASMA_TEST_BOOL(it.IsValid());

    it.Shrink(1, 1);

    PLASMA_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    PLASMA_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    PLASMA_TEST_STRING(it.GetData(tmp), "");
    PLASMA_TEST_BOOL(!it.IsValid());

    it.Shrink(10, 10);

    PLASMA_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    PLASMA_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    PLASMA_TEST_STRING(it.GetData(tmp), "");
    PLASMA_TEST_BOOL(!it.IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChopAwayFirstCharacterUtf8")
  {
    plStringUtf8 utf8(L"О, Господи!");
    plStringView s(utf8.GetData());

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const plUInt32 uiNumCharsBefore = plStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());
      s.ChopAwayFirstCharacterUtf8();
      const plUInt32 uiNumCharsAfter = plStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());

      PLASMA_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // this needs to be true, some code relies on the fact that the start pointer always moves forwards
    PLASMA_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChopAwayFirstCharacterAscii")
  {
    plStringUtf8 utf8(L"Wosn Schmarrn");
    plStringView s("");

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const plUInt32 uiNumCharsBefore = s.GetElementCount();
      s.ChopAwayFirstCharacterAscii();
      const plUInt32 uiNumCharsAfter = s.GetElementCount();

      PLASMA_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // this needs to be true, some code relies on the fact that the start pointer always moves forwards
    PLASMA_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Trim")
  {
    // Empty input
    plStringUtf8 utf8(L"");
    plStringView view(utf8.GetData());
    view.Trim(" \t");
    PLASMA_TEST_BOOL(view.IsEqual(plStringUtf8(L"").GetData()));
    view.Trim(nullptr, " \t");
    PLASMA_TEST_BOOL(view.IsEqual(plStringUtf8(L"").GetData()));
    view.Trim(" \t", nullptr);
    PLASMA_TEST_BOOL(view.IsEqual(plStringUtf8(L"").GetData()));

    // Clear all from one side
    plStringUtf8 sUnicode(L"私はクリストハさんです");
    view = sUnicode.GetData();
    view.Trim(nullptr, sUnicode.GetData());
    PLASMA_TEST_BOOL(view.IsEqual(""));
    view = sUnicode.GetData();
    view.Trim(sUnicode.GetData(), nullptr);
    PLASMA_TEST_BOOL(view.IsEqual(""));

    // Clear partial side
    sUnicode = L"ですですですAにぱにぱにぱ";
    view = sUnicode.GetData();
    view.Trim(nullptr, plStringUtf8(L"にぱ").GetData());
    sUnicode = L"ですですですA";
    PLASMA_TEST_BOOL(view.IsEqual(sUnicode.GetData()));
    view.Trim(plStringUtf8(L"です").GetData(), nullptr);
    PLASMA_TEST_BOOL(view.IsEqual(plStringUtf8(L"A").GetData()));

    sUnicode = L"ですですですAにぱにぱにぱ";
    view = sUnicode.GetData();
    view.Trim(plStringUtf8(L"ですにぱ").GetData());
    PLASMA_TEST_BOOL(view.IsEqual(plStringUtf8(L"A").GetData()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TrimWordStart")
  {
    plStringView sb;

    {
      sb = "<test>abc<test>";
      PLASMA_TEST_BOOL(sb.TrimWordStart("<test>"));
      PLASMA_TEST_STRING(sb, "abc<test>");
      PLASMA_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      PLASMA_TEST_STRING(sb, "abc<test>");
    }

    {
      sb = "<test><tut><test><test><tut>abc<tut><test>";
      PLASMA_TEST_BOOL(!sb.TrimWordStart("<tut>"));
      PLASMA_TEST_BOOL(sb.TrimWordStart("<test>"));
      PLASMA_TEST_BOOL(sb.TrimWordStart("<tut>"));
      PLASMA_TEST_BOOL(sb.TrimWordStart("<test>"));
      PLASMA_TEST_BOOL(sb.TrimWordStart("<test>"));
      PLASMA_TEST_BOOL(sb.TrimWordStart("<tut>"));
      PLASMA_TEST_STRING(sb, "abc<tut><test>");
      PLASMA_TEST_BOOL(sb.TrimWordStart("<tut>") == false);
      PLASMA_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      PLASMA_TEST_STRING(sb, "abc<tut><test>");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>abc";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      PLASMA_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      PLASMA_TEST_STRING(sb, "");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TrimWordEnd")
  {
    plStringView sb;

    {
      sb = "<test>abc<test>";
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<test>"));
      PLASMA_TEST_STRING(sb, "<test>abc");
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      PLASMA_TEST_STRING(sb, "<test>abc");
    }

    {
      sb = "<tut><test>abc<test><tut><test><test><tut>";
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<test>"));
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<test>"));
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<test>"));
      PLASMA_TEST_STRING(sb, "<tut><test>abc");
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<tut>") == false);
      PLASMA_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      PLASMA_TEST_STRING(sb, "<tut><test>abc");
    }

    {
      sb = "abc<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      PLASMA_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      PLASMA_TEST_STRING(sb, "");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Split")
  {
    plStringView s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    plDeque<plStringView> SubStrings;

    s.Split(false, SubStrings, ",", "|", "<>");

    PLASMA_TEST_INT(SubStrings.GetCount(), 7);
    PLASMA_TEST_BOOL(SubStrings[0] == "abc");
    PLASMA_TEST_BOOL(SubStrings[1] == "def");
    PLASMA_TEST_BOOL(SubStrings[2] == "ghi");
    PLASMA_TEST_BOOL(SubStrings[3] == "jkl");
    PLASMA_TEST_BOOL(SubStrings[4] == "mno");
    PLASMA_TEST_BOOL(SubStrings[5] == "pqr");
    PLASMA_TEST_BOOL(SubStrings[6] == "stu");

    s.Split(true, SubStrings, ",", "|", "<>");

    PLASMA_TEST_INT(SubStrings.GetCount(), 10);
    PLASMA_TEST_BOOL(SubStrings[0] == "");
    PLASMA_TEST_BOOL(SubStrings[1] == "abc");
    PLASMA_TEST_BOOL(SubStrings[2] == "def");
    PLASMA_TEST_BOOL(SubStrings[3] == "ghi");
    PLASMA_TEST_BOOL(SubStrings[4] == "");
    PLASMA_TEST_BOOL(SubStrings[5] == "");
    PLASMA_TEST_BOOL(SubStrings[6] == "jkl");
    PLASMA_TEST_BOOL(SubStrings[7] == "mno");
    PLASMA_TEST_BOOL(SubStrings[8] == "pqr");
    PLASMA_TEST_BOOL(SubStrings[9] == "stu");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HasAnyExtension")
  {
    plStringView p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    PLASMA_TEST_BOOL(!p.HasAnyExtension());
    PLASMA_TEST_BOOL(!p.HasAnyExtension());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HasExtension")
  {
    plStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.HasExtension(".Extension"));

    p = "This/Is\\My//Path.dot\\file.ext";
    PLASMA_TEST_BOOL(p.HasExtension("EXT"));

    p = "This/Is\\My//Path.dot\\file.ext";
    PLASMA_TEST_BOOL(!p.HasExtension("NEXT"));

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(!p.HasExtension(".Ext"));

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(!p.HasExtension("sion"));

    p = "";
    PLASMA_TEST_BOOL(!p.HasExtension("ext"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileExtension")
  {
    plStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    PLASMA_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    PLASMA_TEST_BOOL(p.GetFileExtension() == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileNameAndExtension")
  {
    plStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.GetFileNameAndExtension() == "file.extension");

    p = "This/Is\\My//Path.dot\\.extension";
    PLASMA_TEST_BOOL(p.GetFileNameAndExtension() == ".extension");

    p = "This/Is\\My//Path.dot\\file";
    PLASMA_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "\\file";
    PLASMA_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "";
    PLASMA_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "/";
    PLASMA_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "This/Is\\My//Path.dot\\";
    PLASMA_TEST_BOOL(p.GetFileNameAndExtension() == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileName")
  {
    plStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.GetFileName() == "file");

    p = "This/Is\\My//Path.dot\\file";
    PLASMA_TEST_BOOL(p.GetFileName() == "file");

    p = "\\file";
    PLASMA_TEST_BOOL(p.GetFileName() == "file");

    p = "";
    PLASMA_TEST_BOOL(p.GetFileName() == "");

    p = "/";
    PLASMA_TEST_BOOL(p.GetFileName() == "");

    p = "This/Is\\My//Path.dot\\";
    PLASMA_TEST_BOOL(p.GetFileName() == "");

    // so far we treat file and folders whose names start with a '.' as extensions
    p = "This/Is\\My//Path.dot\\.stupidfile";
    PLASMA_TEST_BOOL(p.GetFileName() == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileDirectory")
  {
    plStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\.extension";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\file";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "\\file";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "\\");

    p = "";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "");

    p = "/";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "/");

    p = "This/Is\\My//Path.dot\\";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This";
    PLASMA_TEST_BOOL(p.GetFileDirectory() == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsAbsolutePath / IsRelativePath / IsRootedPath")
  {
    plStringView p;

    p = "";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    p = "C:\\temp.stuff";
    PLASMA_TEST_BOOL(p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "C:/temp.stuff";
    PLASMA_TEST_BOOL(p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "\\\\myserver\\temp.stuff";
    PLASMA_TEST_BOOL(p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "\\myserver\\temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath()); // neither absolute nor relativ, just stupid
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath()); // bloed
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "\\temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath()); // bloed
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = ":MyDataDir\bla";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath());
    PLASMA_TEST_BOOL(p.IsRootedPath());

    p = ":\\MyDataDir\bla";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath());
    PLASMA_TEST_BOOL(p.IsRootedPath());

    p = ":/MyDataDir/bla";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath());
    PLASMA_TEST_BOOL(p.IsRootedPath());

#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)

    p = "C:\\temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    PLASMA_TEST_BOOL(p.IsAbsolutePath());
    PLASMA_TEST_BOOL(!p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    PLASMA_TEST_BOOL(!p.IsAbsolutePath());
    PLASMA_TEST_BOOL(p.IsRelativePath());
    PLASMA_TEST_BOOL(!p.IsRootedPath());

#else
#  error "Unknown platform."
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetRootedPathRootName")
  {
    plStringView p;

    p = ":root\\bla";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":root/bla";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://root/bla";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":/\\/root\\/bla";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://\\root";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "noroot\\bla";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "C:\\noroot/bla";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "/noroot/bla";
    PLASMA_TEST_BOOL(p.GetRootedPathRootName() == "");
  }
}
