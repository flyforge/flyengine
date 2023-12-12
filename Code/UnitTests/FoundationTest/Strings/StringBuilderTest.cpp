#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

// this file takes ages to compile in a Release build
// since we don't care for runtime performance, just disable all optimizations
#pragma optimize("", off)

PLASMA_CREATE_SIMPLE_TEST(Strings, StringBuilder)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(empty)")
  {
    plStringBuilder s;

    PLASMA_TEST_BOOL(s.IsEmpty());
    PLASMA_TEST_INT(s.GetCharacterCount(), 0);
    PLASMA_TEST_INT(s.GetElementCount(), 0);
    PLASMA_TEST_BOOL(s.IsPureASCII());
    PLASMA_TEST_BOOL(s == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(Utf8)")
  {
    plStringUtf8 sUtf8(L"abc äöü € def");
    plStringBuilder s(sUtf8.GetData());

    PLASMA_TEST_BOOL(s.GetData() != sUtf8.GetData());
    PLASMA_TEST_BOOL(s == sUtf8.GetData());
    PLASMA_TEST_INT(s.GetElementCount(), 18);
    PLASMA_TEST_INT(s.GetCharacterCount(), 13);
    PLASMA_TEST_BOOL(!s.IsPureASCII());

    plStringBuilder s2("test test");

    PLASMA_TEST_BOOL(s2 == "test test");
    PLASMA_TEST_INT(s2.GetElementCount(), 9);
    PLASMA_TEST_INT(s2.GetCharacterCount(), 9);
    PLASMA_TEST_BOOL(s2.IsPureASCII());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(wchar_t)")
  {
    plStringUtf8 sUtf8(L"abc äöü € def");
    plStringBuilder s(L"abc äöü € def");

    PLASMA_TEST_BOOL(s == sUtf8.GetData());
    PLASMA_TEST_INT(s.GetElementCount(), 18);
    PLASMA_TEST_INT(s.GetCharacterCount(), 13);
    PLASMA_TEST_BOOL(!s.IsPureASCII());

    plStringBuilder s2(L"test test");

    PLASMA_TEST_BOOL(s2 == "test test");
    PLASMA_TEST_INT(s2.GetElementCount(), 9);
    PLASMA_TEST_INT(s2.GetCharacterCount(), 9);
    PLASMA_TEST_BOOL(s2.IsPureASCII());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(copy)")
  {
    plStringUtf8 sUtf8(L"abc äöü € def");
    plStringBuilder s(L"abc äöü € def");
    plStringBuilder s2(s);

    PLASMA_TEST_BOOL(s2 == sUtf8.GetData());
    PLASMA_TEST_INT(s2.GetElementCount(), 18);
    PLASMA_TEST_INT(s2.GetCharacterCount(), 13);
    PLASMA_TEST_BOOL(!s2.IsPureASCII());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(StringView)")
  {
    plStringUtf8 sUtf8(L"abc äöü € def");

    plStringView it(sUtf8.GetData() + 2, sUtf8.GetData() + 8);

    plStringBuilder s(it);

    PLASMA_TEST_INT(s.GetElementCount(), 6);
    PLASMA_TEST_INT(s.GetCharacterCount(), 4);
    PLASMA_TEST_BOOL(!s.IsPureASCII());
    PLASMA_TEST_BOOL(s == plStringUtf8(L"c äö").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor(multiple)")
  {
    plStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    plStringUtf8 sUtf2(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");

    plStringBuilder sb(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData());

    PLASMA_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=(Utf8)")
  {
    plStringUtf8 sUtf8(L"abc äöü € def");
    plStringBuilder s("bla");
    s = sUtf8.GetData();

    PLASMA_TEST_BOOL(s.GetData() != sUtf8.GetData());
    PLASMA_TEST_BOOL(s == sUtf8.GetData());
    PLASMA_TEST_INT(s.GetElementCount(), 18);
    PLASMA_TEST_INT(s.GetCharacterCount(), 13);
    PLASMA_TEST_BOOL(!s.IsPureASCII());

    plStringBuilder s2("bla");
    s2 = "test test";

    PLASMA_TEST_BOOL(s2 == "test test");
    PLASMA_TEST_INT(s2.GetElementCount(), 9);
    PLASMA_TEST_INT(s2.GetCharacterCount(), 9);
    PLASMA_TEST_BOOL(s2.IsPureASCII());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=(wchar_t)")
  {
    plStringUtf8 sUtf8(L"abc äöü € def");
    plStringBuilder s("bla");
    s = L"abc äöü € def";

    PLASMA_TEST_BOOL(s == sUtf8.GetData());
    PLASMA_TEST_INT(s.GetElementCount(), 18);
    PLASMA_TEST_INT(s.GetCharacterCount(), 13);
    PLASMA_TEST_BOOL(!s.IsPureASCII());

    plStringBuilder s2("bla");
    s2 = L"test test";

    PLASMA_TEST_BOOL(s2 == "test test");
    PLASMA_TEST_INT(s2.GetElementCount(), 9);
    PLASMA_TEST_INT(s2.GetCharacterCount(), 9);
    PLASMA_TEST_BOOL(s2.IsPureASCII());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=(copy)")
  {
    plStringUtf8 sUtf8(L"abc äöü € def");
    plStringBuilder s(L"abc äöü € def");
    plStringBuilder s2;
    s2 = s;

    PLASMA_TEST_BOOL(s2 == sUtf8.GetData());
    PLASMA_TEST_INT(s2.GetElementCount(), 18);
    PLASMA_TEST_INT(s2.GetCharacterCount(), 13);
    PLASMA_TEST_BOOL(!s2.IsPureASCII());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=(StringView)")
  {
    plStringBuilder s("abcdefghi");
    plStringView it(s.GetData() + 2, s.GetData() + 8);
    it.SetStartPosition(s.GetData() + 3);

    s = it;

    PLASMA_TEST_BOOL(s == "defgh");
    PLASMA_TEST_INT(s.GetElementCount(), 5);
    PLASMA_TEST_INT(s.GetCharacterCount(), 5);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "convert to plStringView")
  {
    plStringBuilder s(L"aölsdföasld");
    plStringBuilder tmp;

    plStringView sv = s;

    PLASMA_TEST_STRING(sv.GetData(tmp), plStringUtf8(L"aölsdföasld").GetData());
    PLASMA_TEST_BOOL(sv == plStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    PLASMA_TEST_STRING(sv.GetStartPointer(), "abcdef");
    PLASMA_TEST_BOOL(sv == "abcdef");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    plStringBuilder s(L"abc äöü € def");

    PLASMA_TEST_BOOL(!s.IsEmpty());
    PLASMA_TEST_BOOL(!s.IsPureASCII());

    s.Clear();
    PLASMA_TEST_BOOL(s.IsEmpty());
    PLASMA_TEST_INT(s.GetElementCount(), 0);
    PLASMA_TEST_INT(s.GetCharacterCount(), 0);
    PLASMA_TEST_BOOL(s.IsPureASCII());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetElementCount / GetCharacterCount / IsPureASCII")
  {
    plStringBuilder s(L"abc äöü € def");

    PLASMA_TEST_BOOL(!s.IsPureASCII());
    PLASMA_TEST_INT(s.GetElementCount(), 18);
    PLASMA_TEST_INT(s.GetCharacterCount(), 13);

    s = "abc";

    PLASMA_TEST_BOOL(s.IsPureASCII());
    PLASMA_TEST_INT(s.GetElementCount(), 3);
    PLASMA_TEST_INT(s.GetCharacterCount(), 3);

    s = L"Hällo! I love €";

    PLASMA_TEST_BOOL(!s.IsPureASCII());
    PLASMA_TEST_INT(s.GetElementCount(), 18);
    PLASMA_TEST_INT(s.GetCharacterCount(), 15);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Append(single unicode char)")
  {
    plStringUtf32 u32(L"äöüß");

    plStringBuilder s("abc");
    PLASMA_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(u32.GetData()[0]);
    PLASMA_TEST_INT(s.GetCharacterCount(), 4);

    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcä").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Prepend(single unicode char)")
  {
    plStringUtf32 u32(L"äöüß");

    plStringBuilder s("abc");
    PLASMA_TEST_INT(s.GetCharacterCount(), 3);
    s.Prepend(u32.GetData()[0]);
    PLASMA_TEST_INT(s.GetCharacterCount(), 4);

    PLASMA_TEST_BOOL(s == plStringUtf8(L"äabc").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Append(char)")
  {
    plStringBuilder s("abc");
    PLASMA_TEST_INT(s.GetCharacterCount(), 3);
    s.Append("de", "fg", "hi", plStringUtf8(L"öä").GetData(), "jk", plStringUtf8(L"ü€").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 15);

    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, "b", nullptr, "d", nullptr, plStringUtf8(L"ü€").GetData());
    PLASMA_TEST_BOOL(s == plStringUtf8(L"pupsbdü€").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Append(wchar_t)")
  {
    plStringBuilder s("abc");
    PLASMA_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");
    PLASMA_TEST_INT(s.GetCharacterCount(), 15);

    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    PLASMA_TEST_BOOL(s == plStringUtf8(L"pupsbdü€").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Append(multiple)")
  {
    plStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    plStringUtf8 sUtf2(L"Test⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    plStringBuilder sb("Test");
    sb.Append(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    PLASMA_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Set(multiple)")
  {
    plStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    plStringUtf8 sUtf2(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    plStringBuilder sb("Test");
    sb.Set(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    PLASMA_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AppendFormat")
  {
    plStringBuilder s("abc");
    s.AppendFormat("Test{0}{1}{2}", 42, "foo", plStringUtf8(L"bär").GetData());

    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcTest42foobär").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Prepend(char)")
  {
    plStringBuilder s("abc");
    s.Prepend("de", "fg", "hi", plStringUtf8(L"öä").GetData(), "jk", plStringUtf8(L"ü€").GetData());

    PLASMA_TEST_BOOL(s == plStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, "b", nullptr, "d", nullptr, plStringUtf8(L"ü€").GetData());
    PLASMA_TEST_BOOL(s == plStringUtf8(L"bdü€pups").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Prepend(wchar_t)")
  {
    plStringBuilder s("abc");
    s.Prepend(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");

    PLASMA_TEST_BOOL(s == plStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    PLASMA_TEST_BOOL(s == plStringUtf8(L"bdü€pups").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PrependFormat")
  {
    plStringBuilder s("abc");
    s.PrependFormat("Test{0}{1}{2}", 42, "foo", plStringUtf8(L"bär").GetData());

    PLASMA_TEST_BOOL(s == plStringUtf8(L"Test42foobärabc").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Printf")
  {
    plStringBuilder s("abc");
    s.Printf("Test%i%s%s", 42, "foo", plStringUtf8(L"bär").GetData());

    PLASMA_TEST_BOOL(s == plStringUtf8(L"Test42foobär").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Format")
  {
    plStringBuilder s("abc");
    s.Format("Test{0}{1}{2}", 42, "foo", plStringUtf8(L"bär").GetData());

    PLASMA_TEST_BOOL(s == plStringUtf8(L"Test42foobär").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ToUpper")
  {
    plStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.ToUpper();
    PLASMA_TEST_BOOL(s == plStringUtf8(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ToLower")
  {
    plStringBuilder s(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß");
    s.ToLower();
    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Shrink")
  {
    plStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.Shrink(5, 3);

    PLASMA_TEST_BOOL(s == plStringUtf8(L"fghijklmnopqrstuvwxyzäö").GetData());

    s.Shrink(9, 7);
    PLASMA_TEST_BOOL(s == plStringUtf8(L"opqrstu").GetData());

    s.Shrink(3, 2);
    PLASMA_TEST_BOOL(s == plStringUtf8(L"rs").GetData());

    s.Shrink(1, 0);
    PLASMA_TEST_BOOL(s == plStringUtf8(L"s").GetData());

    s.Shrink(0, 0);
    PLASMA_TEST_BOOL(s == plStringUtf8(L"s").GetData());

    s.Shrink(0, 1);
    PLASMA_TEST_BOOL(s == plStringUtf8(L"").GetData());

    s.Shrink(10, 0);
    PLASMA_TEST_BOOL(s == plStringUtf8(L"").GetData());

    s.Shrink(0, 10);
    PLASMA_TEST_BOOL(s == plStringUtf8(L"").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reserve")
  {
    plHeapAllocator allocator("reserve test allocator");
    plStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß", &allocator);
    plUInt32 characterCountBefore = s.GetCharacterCount();

    s.Reserve(2048);

    PLASMA_TEST_BOOL(s.GetCharacterCount() == characterCountBefore);

    plUInt64 iNumAllocs = allocator.GetStats().m_uiNumAllocations;
    s.Append("blablablablablablablablablablablablablablablablablablablablablablablablablablablablablabla");
    PLASMA_TEST_BOOL(iNumAllocs == allocator.GetStats().m_uiNumAllocations);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Convert to StringView")
  {
    plStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    plStringView it = s;

    PLASMA_TEST_BOOL(it.StartsWith(plStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
    PLASMA_TEST_BOOL(it.EndsWith(plStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChangeCharacter")
  {
    plStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    plStringUtf8 upr(L"ÄÖÜ€ßABCDEFGHIJKLMNOPQRSTUVWXYZ");
    plStringView view(upr.GetData());

    for (auto it = begin(s); it.IsValid(); ++it, view.Shrink(1, 0))
    {
      s.ChangeCharacter(it, view.GetCharacter());

      PLASMA_TEST_BOOL(it.GetCharacter() == view.GetCharacter()); // iterator reflects the changes
    }

    PLASMA_TEST_BOOL(s == upr.GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 31);
    PLASMA_TEST_INT(s.GetElementCount(), 37);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceSubString")
  {
    plStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    s.ReplaceSubString(s.GetData() + 3, s.GetData() + 7, "DEFG"); // equal length, equal num characters
    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcDEFGhijklmnopqrstuvwxyzäöü€ß").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 31);
    PLASMA_TEST_INT(s.GetElementCount(), 37);

    s.ReplaceSubString(s.GetData() + 7, s.GetData() + 15, ""); // remove
    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcDEFGpqrstuvwxyzäöü€ß").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 23);
    PLASMA_TEST_INT(s.GetElementCount(), 29);

    s.ReplaceSubString(s.GetData() + 17, s.GetData() + 22, "blablub"); // make longer
    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcDEFGpqrstuvwxyblablubü€ß").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 27);
    PLASMA_TEST_INT(s.GetElementCount(), 31);

    s.ReplaceSubString(s.GetData() + 22, s.GetData() + 22, plStringUtf8(L"määh!").GetData()); // insert
    PLASMA_TEST_BOOL(s == plStringUtf8(L"abcDEFGpqrstuvwxyblablmääh!ubü€ß").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 32);
    PLASMA_TEST_INT(s.GetElementCount(), 38);

    s.ReplaceSubString(s.GetData(), s.GetData() + 10, nullptr); // remove at front
    PLASMA_TEST_BOOL(s == plStringUtf8(L"stuvwxyblablmääh!ubü€ß").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 22);
    PLASMA_TEST_INT(s.GetElementCount(), 28);

    s.ReplaceSubString(s.GetData() + 18, s.GetData() + 28, nullptr); // remove at back
    PLASMA_TEST_BOOL(s == plStringUtf8(L"stuvwxyblablmääh").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 16);
    PLASMA_TEST_INT(s.GetElementCount(), 18);

    s.ReplaceSubString(s.GetData(), s.GetData() + 18, nullptr); // clear
    PLASMA_TEST_BOOL(s == plStringUtf8(L"").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 0);
    PLASMA_TEST_INT(s.GetElementCount(), 0);

    const char* szInsert = "abc def ghi";

    s.ReplaceSubString(s.GetData(), s.GetData(), plStringView(szInsert, szInsert + 7)); // partial insert into empty
    PLASMA_TEST_BOOL(s == plStringUtf8(L"abc def").GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), 7);
    PLASMA_TEST_INT(s.GetElementCount(), 7);

    // insert very large block
    s = plStringBuilder("a"); // hard reset to keep buffer small
    plString insertString("omfg this string is so long it possibly won't never ever ever ever fit into the current buffer - this will "
                          "hopefully lead to a buffer resize :)"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................");
    s.ReplaceSubString(s.GetData(), s.GetData() + s.GetElementCount(), insertString.GetData());
    PLASMA_TEST_BOOL(s == insertString.GetData());
    PLASMA_TEST_INT(s.GetCharacterCount(), insertString.GetCharacterCount());
    PLASMA_TEST_INT(s.GetElementCount(), insertString.GetElementCount());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert")
  {
    plStringBuilder s;

    s.Insert(s.GetData(), "test");
    PLASMA_TEST_BOOL(s == "test");

    s.Insert(s.GetData() + 2, "TUT");
    PLASMA_TEST_BOOL(s == "teTUTst");

    s.Insert(s.GetData(), "MOEP");
    PLASMA_TEST_BOOL(s == "MOEPteTUTst");

    s.Insert(s.GetData() + s.GetElementCount(), "hompf");
    PLASMA_TEST_BOOL(s == "MOEPteTUTsthompf");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove")
  {
    plStringBuilder s("MOEPteTUTsthompf");

    s.Remove(s.GetData() + 11, s.GetData() + s.GetElementCount());
    PLASMA_TEST_BOOL(s == "MOEPteTUTst");

    s.Remove(s.GetData(), s.GetData() + 4);
    PLASMA_TEST_BOOL(s == "teTUTst");

    s.Remove(s.GetData() + 2, s.GetData() + 5);
    PLASMA_TEST_BOOL(s == "test");

    s.Remove(s.GetData(), s.GetData() + s.GetElementCount());
    PLASMA_TEST_BOOL(s == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceFirst")
  {
    plStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst("def", "BLOED");
    PLASMA_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED", s.GetData() + 15);
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("def", "OEDE");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("abc", "BLOEDE");
    PLASMA_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    PLASMA_TEST_BOOL(s == "weg");

    s.ReplaceFirst("weg", nullptr);
    PLASMA_TEST_BOOL(s == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceLast")
  {
    plStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast("abc", "ABC");
    PLASMA_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    PLASMA_TEST_BOOL(s == "abc def ABC def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    PLASMA_TEST_BOOL(s == "ABC def ABC def ghi ABC ghi");

    s.ReplaceLast("ghi", "GHI", s.GetData() + 24);
    PLASMA_TEST_BOOL(s == "ABC def ABC def GHI ABC ghi");

    s.ReplaceLast("i", "I");
    PLASMA_TEST_BOOL(s == "ABC def ABC def GHI ABC ghI");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceAll")
  {
    plStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll("abc", "TEST");
    PLASMA_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "def");
    PLASMA_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    PLASMA_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    PLASMA_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll("def", " ");
    PLASMA_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll(" ", "");
    PLASMA_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll("TEST", "a");
    PLASMA_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll("hi", "hihi");
    PLASMA_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll("ag", " ");
    PLASMA_TEST_BOOL(s == "a hihi hihi");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceFirst_NoCase")
  {
    plStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst_NoCase("DEF", "BLOED");
    PLASMA_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED", s.GetData() + 15);
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("DEF", "OEDE");
    PLASMA_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("ABC", "BLOEDE");
    PLASMA_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    PLASMA_TEST_BOOL(s == "weg");

    s.ReplaceFirst_NoCase("WEG", nullptr);
    PLASMA_TEST_BOOL(s == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceLast_NoCase")
  {
    plStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast_NoCase("abc", "ABC");
    PLASMA_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("aBc", "ABC");
    PLASMA_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("ABC", "ABC");
    PLASMA_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("GHI", "GHI", s.GetData() + 24);
    PLASMA_TEST_BOOL(s == "abc def abc def GHI ABC ghi");

    s.ReplaceLast_NoCase("I", "I");
    PLASMA_TEST_BOOL(s == "abc def abc def GHI ABC ghI");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceAll_NoCase")
  {
    plStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll_NoCase("ABC", "TEST");
    PLASMA_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "def");
    PLASMA_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    PLASMA_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    PLASMA_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", " ");
    PLASMA_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll_NoCase(" ", "");
    PLASMA_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll_NoCase("teST", "a");
    PLASMA_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll_NoCase("hI", "hihi");
    PLASMA_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll_NoCase("Ag", " ");
    PLASMA_TEST_BOOL(s == "a hihi hihi");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceWholeWord")
  {
    plStringBuilder s = "abcd abc abcd abc dabc abc";

    PLASMA_TEST_BOOL(s.ReplaceWholeWord("abc", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord("abc", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc abc");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord("abc", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord("abc", "def", plStringUtils::IsWordDelimiter_English) == nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "def def abcd def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", plStringUtils::IsWordDelimiter_English) == nullptr);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceWholeWord_NoCase")
  {
    plStringBuilder s = "abcd abc abcd abc dabc abc";

    PLASMA_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc abc");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", plStringUtils::IsWordDelimiter_English) == nullptr);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABCd", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "def def abcd def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord_NoCase("aBCD", "def", plStringUtils::IsWordDelimiter_English) != nullptr);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");

    PLASMA_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABcd", "def", plStringUtils::IsWordDelimiter_English) == nullptr);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceWholeWordAll")
  {
    plStringBuilder s = "abcd abc abcd abc dabc abc";

    PLASMA_TEST_INT(s.ReplaceWholeWordAll("abc", "def", plStringUtils::IsWordDelimiter_English), 3);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_INT(s.ReplaceWholeWordAll("abc", "def", plStringUtils::IsWordDelimiter_English), 0);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", plStringUtils::IsWordDelimiter_English), 2);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");

    PLASMA_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", plStringUtils::IsWordDelimiter_English), 0);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReplaceWholeWordAll_NoCase")
  {
    plStringBuilder s = "abcd abc abcd abc dabc abc";

    PLASMA_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", plStringUtils::IsWordDelimiter_English), 3);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", plStringUtils::IsWordDelimiter_English), 0);
    PLASMA_TEST_BOOL(s == "abcd def abcd def dabc def");

    PLASMA_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", plStringUtils::IsWordDelimiter_English), 2);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");

    PLASMA_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", plStringUtils::IsWordDelimiter_English), 0);
    PLASMA_TEST_BOOL(s == "def def def def dabc def");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "teset")
  {
    const char* sz = "abc def";
    plStringView it(sz);

    plStringBuilder s = it;
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Split")
  {
    plStringBuilder s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    plHybridArray<plStringView, 32> SubStrings;

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


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeCleanPath")
  {
    plStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "C:/temp/temp/tut");

    p = "\\temp/temp//tut\\\\";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "/temp/temp/tut/");

    p = "\\";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "/");

    p = "file";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "file");

    p = "C:\\temp/..//tut";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "C:/tut");

    p = "C:\\temp/..";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "C:/temp/..");

    p = "C:\\temp/..\\";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "C:/");

    p = "\\//temp/../bla\\\\blub///..\\temp//tut/tat/..\\\\..\\//ploep";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "//bla/temp/ploep");

    p = "a/b/c/../../../../e/f";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "../e/f");

    p = "/../../a/../../e/f";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "../../e/f");

    p = "/../../a/../../e/f/../";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "../../e/");

    p = "/../../a/../../e/f/..";
    p.MakeCleanPath();
    PLASMA_TEST_BOOL(p == "../../e/f/..");

    p = "\\//temp/./bla\\\\blub///.\\temp//tut/tat/..\\.\\.\\//ploep";
    p.MakeCleanPath();
    PLASMA_TEST_STRING(p.GetData(), "//temp/bla/blub/temp/tut/ploep");

    p = "./";
    p.MakeCleanPath();
    PLASMA_TEST_STRING(p.GetData(), "");

    p = "/./././";
    p.MakeCleanPath();
    PLASMA_TEST_STRING(p.GetData(), "/");

    p = "./.././";
    p.MakeCleanPath();
    PLASMA_TEST_STRING(p.GetData(), "../");

    // more than two dots are invalid, so the should be kept as is
    p = "./..././abc/...\\def";
    p.MakeCleanPath();
    PLASMA_TEST_STRING(p.GetData(), ".../abc/.../def");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PathParentDirectory")
  {
    plStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.PathParentDirectory();
    PLASMA_TEST_BOOL(p == "C:/temp/temp/");

    p = "C:\\temp/temp//tut\\\\";
    p.PathParentDirectory();
    PLASMA_TEST_BOOL(p == "C:/temp/temp/");

    p = "file";
    p.PathParentDirectory();
    PLASMA_TEST_BOOL(p == "");

    p = "/file";
    p.PathParentDirectory();
    PLASMA_TEST_BOOL(p == "/");

    p = "C:\\temp/..//tut";
    p.PathParentDirectory();
    PLASMA_TEST_BOOL(p == "C:/");

    p = "file";
    p.PathParentDirectory(3);
    PLASMA_TEST_BOOL(p == "../../");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "AppendPath")
  {
    plStringBuilder p;

    p = "this/is\\my//path";
    p.AppendPath("orly/nowai");
    PLASMA_TEST_BOOL(p == "this/is\\my//path/orly/nowai");

    p = "this/is\\my//path///";
    p.AppendPath("orly/nowai");
    PLASMA_TEST_BOOL(p == "this/is\\my//path///orly/nowai");

    p = "";
    p.AppendPath("orly/nowai");
    PLASMA_TEST_BOOL(p == "orly/nowai");

    // It should be valid to append an absolute path to an empty string.
    {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
      const char* szAbsPath = "C:\\folder";
      const char* szAbsPathAppendResult = "C:\\folder/File.ext";
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX) || PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
      const char* szAbsPath = "/folder";
      const char* szAbsPathAppendResult = "/folder/File.ext";
#else
#  error "An absolute path example must be defined for the 'AppendPath' test for each platform!"
#endif

      p = "";
      p.AppendPath(szAbsPath, "File.ext");
      PLASMA_TEST_BOOL(p == szAbsPathAppendResult);
    }

    p = "bla";
    p.AppendPath("");
    PLASMA_TEST_BOOL(p == "bla");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChangeFileName")
  {
    plStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileName("bla");
    PLASMA_TEST_BOOL(p == "C:/test/test/bla.ext");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileName("toeff");
    PLASMA_TEST_BOOL(p == "test/test/tut/toeff.toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileName("toeff");
    PLASMA_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf/";
    p.ChangeFileName("toeff");
    PLASMA_TEST_BOOL(p == "test/test/tut/murpf/toeff"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.extension"; // folders that start with a dot must be considered to be empty filenames with an extension
    p.ChangeFileName("toeff");
    PLASMA_TEST_BOOL(p == "test/test/tut/murpf/toeff.extension");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileName("toeff");
    PLASMA_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChangeFileNameAndExtension")
  {
    plStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileNameAndExtension("bla.pups");
    PLASMA_TEST_BOOL(p == "C:/test/test/bla.pups");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileNameAndExtension("toeff");
    PLASMA_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileNameAndExtension("toeff.tut");
    PLASMA_TEST_BOOL(p == "test/test/tut/toeff.tut");

    p = "test/test/tut/murpf/";
    p.ChangeFileNameAndExtension("toeff.blo");
    PLASMA_TEST_BOOL(p == "test/test/tut/murpf/toeff.blo"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.extension"; // folders that start with a dot must be considered to be empty filenames with an extension
    p.ChangeFileNameAndExtension("toeff.ext");
    PLASMA_TEST_BOOL(p == "test/test/tut/murpf/toeff.ext");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileNameAndExtension("toeff");
    PLASMA_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ChangeFileExtension")
  {
    plStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("pups");
    PLASMA_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("pups");
    PLASMA_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("");
    PLASMA_TEST_BOOL(p == "C:/test/test/tut.");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("");
    PLASMA_TEST_BOOL(p == "C:/test/test/tut.");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HasAnyExtension")
  {
    plStringBuilder p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    PLASMA_TEST_BOOL(!p.HasAnyExtension());
    PLASMA_TEST_BOOL(!p.HasAnyExtension());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HasExtension")
  {
    plStringBuilder p;

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
    plStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    PLASMA_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    PLASMA_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    PLASMA_TEST_BOOL(p.GetFileExtension() == "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileNameAndExtension")
  {
    plStringBuilder p;

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
    plStringBuilder p;

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
    plStringBuilder p;

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
    plStringBuilder p;

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
    plStringBuilder p;

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsPathBelowFolder")
  {
    plStringBuilder p;

    p = "a/b\\c//d\\\\e/f";
    PLASMA_TEST_BOOL(!p.IsPathBelowFolder("/a/b\\c"));
    PLASMA_TEST_BOOL(p.IsPathBelowFolder("a/b\\c"));
    PLASMA_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//"));
    PLASMA_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//d/\\e\\f")); // equal paths are considered 'below'
    PLASMA_TEST_BOOL(!p.IsPathBelowFolder("a/b\\c//d/\\e\\f/g"));
    PLASMA_TEST_BOOL(p.IsPathBelowFolder("a"));
    PLASMA_TEST_BOOL(!p.IsPathBelowFolder("b"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakeRelativeTo")
  {
    plStringBuilder p;

    p = u8"ä/b\\c/d\\\\e/f/g";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c").Succeeded());
    PLASMA_TEST_BOOL(p == "d/e/f/g");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c").Failed());
    PLASMA_TEST_BOOL(p == "d/e/f/g");

    p = u8"ä/b\\c//d\\\\e/f/g";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c").Succeeded());
    PLASMA_TEST_BOOL(p == "d/e/f/g");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c").Failed());
    PLASMA_TEST_BOOL(p == "d/e/f/g");

    p = u8"ä/b\\c/d\\\\e/f/g";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c/").Succeeded());
    PLASMA_TEST_BOOL(p == "d/e/f/g");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c/").Failed());
    PLASMA_TEST_BOOL(p == "d/e/f/g");

    p = u8"ä/b\\c//d\\\\e/f/g";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c/").Succeeded());
    PLASMA_TEST_BOOL(p == "d/e/f/g");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c/").Failed());
    PLASMA_TEST_BOOL(p == "d/e/f/g");

    p = u8"ä/b\\c//d\\\\e/f/g";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c\\/d/\\e\\f/g").Succeeded());
    PLASMA_TEST_BOOL(p == "");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c\\/d/\\e\\f/g").Failed());
    PLASMA_TEST_BOOL(p == "");

    p = u8"ä/b\\c//d\\\\e/f/g/";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c\\/d//e\\f/g\\h/i").Succeeded());
    PLASMA_TEST_BOOL(p == "../../");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c\\/d//e\\f/g\\h/i").Failed());
    PLASMA_TEST_BOOL(p == "../../");

    p = u8"ä/b\\c//d\\\\e/f/g/j/k";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c\\/d//e\\f/g\\h/i").Succeeded());
    PLASMA_TEST_BOOL(p == "../../j/k");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c\\/d//e\\f/g\\h/i").Failed());
    PLASMA_TEST_BOOL(p == "../../j/k");

    p = u8"ä/b\\c//d\\\\e/f/ge";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c//d/\\e\\f/g\\h/i").Succeeded());
    PLASMA_TEST_BOOL(p == "../../../ge");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c//d/\\e\\f/g\\h/i").Failed());
    PLASMA_TEST_BOOL(p == "../../../ge");

    p = u8"ä/b\\c//d\\\\e/f/g.txt";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c//d//e\\f/g\\h/i").Succeeded());
    PLASMA_TEST_BOOL(p == "../../../g.txt");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c//d//e\\f/g\\h/i").Failed());
    PLASMA_TEST_BOOL(p == "../../../g.txt");

    p = u8"ä/b\\c//d\\\\e/f/g";
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c//d//e\\f/g\\h/i").Succeeded());
    PLASMA_TEST_BOOL(p == "../../");
    PLASMA_TEST_BOOL(p.MakeRelativeTo(u8"ä\\b/c//d//e\\f/g\\h/i").Failed());
    PLASMA_TEST_BOOL(p == "../../");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakePathSeparatorsNative")
  {
    plStringBuilder p;
    p = "This/is\\a/temp\\\\path//to/my///file";

    p.MakePathSeparatorsNative();

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    PLASMA_TEST_STRING(p.GetData(), "This\\is\\a\\temp\\path\\to\\my\\file");
#else
    PLASMA_TEST_STRING(p.GetData(), "This/is/a/temp/path/to/my/file");
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadAll")
  {
    plDefaultMemoryStreamStorage StreamStorage;

    plMemoryStreamWriter MemoryWriter(&StreamStorage);
    plMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText =
      "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, plStringUtils::GetStringElementCount(szText)).IgnoreResult();

    plStringBuilder s;
    s.ReadAll(MemoryReader);

    PLASMA_TEST_BOOL(s == szText);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetSubString_FromTo")
  {
    plStringBuilder sb = "basf";

    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    sb.SetSubString_FromTo(sz + 5, sz + 13);
    PLASMA_TEST_BOOL(sb == "fghijklm");

    sb.SetSubString_FromTo(sz + 17, sz + 30);
    PLASMA_TEST_BOOL(sb == "rstuvwxyz");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetSubString_ElementCount")
  {
    plStringBuilder sb = "basf";

    plStringUtf8 sz(L"aäbcödefügh");

    sb.SetSubString_ElementCount(sz.GetData() + 5, 5);
    PLASMA_TEST_BOOL(sb == plStringUtf8(L"ödef").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetSubString_CharacterCount")
  {
    plStringBuilder sb = "basf";

    plStringUtf8 sz(L"aäbcödefgh");

    sb.SetSubString_CharacterCount(sz.GetData() + 5, 5);
    PLASMA_TEST_BOOL(sb == plStringUtf8(L"ödefg").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveFileExtension")
  {
    plStringBuilder sb = L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺.";

    sb.RemoveFileExtension();
    PLASMA_TEST_STRING_UNICODE(sb.GetData(), plStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺").GetData());

    sb.RemoveFileExtension();
    PLASMA_TEST_STRING_UNICODE(sb.GetData(), plStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴").GetData());

    sb.RemoveFileExtension();
    PLASMA_TEST_STRING_UNICODE(sb.GetData(), plStringUtf8(L"⺅⻩⽇⿕").GetData());

    sb.RemoveFileExtension();
    PLASMA_TEST_STRING_UNICODE(sb.GetData(), plStringUtf8(L"⺅⻩⽇⿕").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Trim")
  {
    // Empty input
    plStringBuilder sb = L"";
    sb.Trim(" \t");
    PLASMA_TEST_STRING(sb.GetData(), plStringUtf8(L"").GetData());
    sb.Trim(nullptr, " \t");
    PLASMA_TEST_STRING(sb.GetData(), plStringUtf8(L"").GetData());
    sb.Trim(" \t", nullptr);
    PLASMA_TEST_STRING(sb.GetData(), plStringUtf8(L"").GetData());

    // Clear all from one side
    auto sUnicode = L"私はクリストハさんです";
    sb = sUnicode;
    sb.Trim(nullptr, plStringUtf8(sUnicode).GetData());
    PLASMA_TEST_STRING(sb.GetData(), "");
    sb = sUnicode;
    sb.Trim(plStringUtf8(sUnicode).GetData(), nullptr);
    PLASMA_TEST_STRING(sb.GetData(), "");

    // Clear partial side
    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(nullptr, plStringUtf8(L"にぱ").GetData());
    PLASMA_TEST_STRING_UNICODE(sb.GetData(), plStringUtf8(L"ですですですA").GetData());
    sb.Trim(plStringUtf8(L"です").GetData(), nullptr);
    PLASMA_TEST_STRING_UNICODE(sb.GetData(), plStringUtf8(L"A").GetData());

    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(plStringUtf8(L"ですにぱ").GetData());
    PLASMA_TEST_STRING(sb.GetData(), plStringUtf8(L"A").GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TrimWordStart")
  {
    plStringBuilder sb;

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
    plStringBuilder sb;

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
}

#pragma optimize("", on)
