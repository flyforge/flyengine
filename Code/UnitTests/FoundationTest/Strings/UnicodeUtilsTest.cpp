#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Strings/String.h>

PLASMA_CREATE_SIMPLE_TEST(Strings, UnicodeUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsASCII")
  {
    // test all ASCII Characters
    for (plUInt32 i = 0; i < 128; ++i)
      PLASMA_TEST_BOOL(plUnicodeUtils::IsASCII(i));

    for (plUInt32 i = 128; i < 0xFFFFF; ++i)
      PLASMA_TEST_BOOL(!plUnicodeUtils::IsASCII(i));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsUtf8StartByte")
  {
    plStringUtf8 s(L"äöü€");
    // ä
    PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf8StartByte(s.GetData()[0]));
    PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8StartByte(s.GetData()[1]));

    // ö
    PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf8StartByte(s.GetData()[2]));
    PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8StartByte(s.GetData()[3]));

    // ü
    PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf8StartByte(s.GetData()[4]));
    PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8StartByte(s.GetData()[5]));

    // €
    PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf8StartByte(s.GetData()[6]));
    PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8StartByte(s.GetData()[7]));
    PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8StartByte(s.GetData()[8]));

    // \0
    PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf8StartByte(s.GetData()[9]));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsUtf8ContinuationByte")
  {
    // all ASCII Characters are not continuation bytes
    for (char i = 0; i < 127; ++i)
    {
      PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8ContinuationByte(i));
    }

    for (plUInt32 i = 0; i < 255u; ++i)
    {
      const char uiContByte = static_cast<char>(0x80 | (i & 0x3f));
      const char uiNoContByte1 = static_cast<char>(i | 0x40);
      const char uiNoContByte2 = static_cast<char>(i | 0xC0);

      PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf8ContinuationByte(uiContByte));
      PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8ContinuationByte(uiNoContByte1));
      PLASMA_TEST_BOOL(!plUnicodeUtils::IsUtf8ContinuationByte(uiNoContByte2));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetUtf8SequenceLength")
  {
    // All ASCII characters are 1 byte in length
    for (char i = 0; i < 127; ++i)
    {
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(i), 1);
    }

    {
      plStringUtf8 s(L"ä");
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      plStringUtf8 s(L"ß");
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      plStringUtf8 s(L"€");
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 3);
    }

    {
      plStringUtf8 s(L"з");
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      plStringUtf8 s(L"г");
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      plStringUtf8 s(L"ы");
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 2);
    }

    {
      plUInt32 u[2] = {L'\u0B87', 0};
      plStringUtf8 s(u);
      PLASMA_TEST_INT(plUnicodeUtils::GetUtf8SequenceLength(s.GetData()[0]), 3);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ConvertUtf8ToUtf32")
  {
    // Just wraps around 'utf8::peek_next'
    // I think we can assume that that works.
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetSizeForCharacterInUtf8")
  {
    // All ASCII characters are 1 byte in length
    for (plUInt32 i = 0; i < 128; ++i)
      PLASMA_TEST_INT(plUnicodeUtils::GetSizeForCharacterInUtf8(i), 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Decode")
  {
    char utf8[] = {(char)0xc3, (char)0xb6, 0};
    plUInt16 utf16[] = {0xf6, 0};
    wchar_t wchar[] = {L'ö', 0};

    char* szUtf8 = &utf8[0];
    plUInt16* szUtf16 = &utf16[0];
    wchar_t* szWChar = &wchar[0];

    plUInt32 uiUtf321 = plUnicodeUtils::DecodeUtf8ToUtf32(szUtf8);
    plUInt32 uiUtf322 = plUnicodeUtils::DecodeUtf16ToUtf32(szUtf16);
    plUInt32 uiUtf323 = plUnicodeUtils::DecodeWCharToUtf32(szWChar);

    PLASMA_TEST_INT(uiUtf321, uiUtf322);
    PLASMA_TEST_INT(uiUtf321, uiUtf323);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Encode")
  {
    char utf8[4] = {0};
    plUInt16 utf16[4] = {0};
    wchar_t wchar[4] = {0};

    char* szUtf8 = &utf8[0];
    plUInt16* szUtf16 = &utf16[0];
    wchar_t* szWChar = &wchar[0];

    plUnicodeUtils::EncodeUtf32ToUtf8(0xf6, szUtf8);
    plUnicodeUtils::EncodeUtf32ToUtf16(0xf6, szUtf16);
    plUnicodeUtils::EncodeUtf32ToWChar(0xf6, szWChar);

    PLASMA_TEST_BOOL(utf8[0] == (char)0xc3 && utf8[1] == (char)0xb6);
    PLASMA_TEST_BOOL(utf16[0] == 0xf6);
    PLASMA_TEST_BOOL(wchar[0] == L'ö');
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MoveToNextUtf8")
  {
    plStringUtf8 s(L"aböäß€de");

    PLASMA_TEST_INT(s.GetElementCount(), 13);

    const char* sz = s.GetData();

    // test how far it skips ahead

    plUnicodeUtils::MoveToNextUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[1]);

    plUnicodeUtils::MoveToNextUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[2]);

    plUnicodeUtils::MoveToNextUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[4]);

    plUnicodeUtils::MoveToNextUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[6]);

    plUnicodeUtils::MoveToNextUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[8]);

    plUnicodeUtils::MoveToNextUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[11]);

    plUnicodeUtils::MoveToNextUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[12]);

    sz = s.GetData();
    const char* szEnd = s.GetView().GetEndPointer();


    plUnicodeUtils::MoveToNextUtf8(sz, szEnd);
    PLASMA_TEST_BOOL(sz == &s.GetData()[1]);

    plUnicodeUtils::MoveToNextUtf8(sz, szEnd);
    PLASMA_TEST_BOOL(sz == &s.GetData()[2]);

    plUnicodeUtils::MoveToNextUtf8(sz, szEnd);
    PLASMA_TEST_BOOL(sz == &s.GetData()[4]);

    plUnicodeUtils::MoveToNextUtf8(sz, szEnd);
    PLASMA_TEST_BOOL(sz == &s.GetData()[6]);

    plUnicodeUtils::MoveToNextUtf8(sz, szEnd);
    PLASMA_TEST_BOOL(sz == &s.GetData()[8]);

    plUnicodeUtils::MoveToNextUtf8(sz, szEnd);
    PLASMA_TEST_BOOL(sz == &s.GetData()[11]);

    plUnicodeUtils::MoveToNextUtf8(sz, szEnd);
    PLASMA_TEST_BOOL(sz == &s.GetData()[12]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MoveToPriorUtf8")
  {
    plStringUtf8 s(L"aböäß€de");

    const char* sz = &s.GetData()[13];

    PLASMA_TEST_INT(s.GetElementCount(), 13);

    // test how far it skips ahead

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[12]);

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[11]);

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[8]);

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[6]);

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[4]);

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[2]);

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[1]);

    plUnicodeUtils::MoveToPriorUtf8(sz);
    PLASMA_TEST_BOOL(sz == &s.GetData()[0]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SkipUtf8Bom")
  {
    // C++ is really stupid, chars are signed, but Utf8 only works with unsigned values ... argh!

    char szWithBom[] = {(char)0xef, (char)0xbb, (char)0xbf, 'a'};
    char szNoBom[] = {'a'};
    const char* pString = szWithBom;

    PLASMA_TEST_BOOL(plUnicodeUtils::SkipUtf8Bom(pString) == true);
    PLASMA_TEST_BOOL(pString == &szWithBom[3]);

    pString = szNoBom;

    PLASMA_TEST_BOOL(plUnicodeUtils::SkipUtf8Bom(pString) == false);
    PLASMA_TEST_BOOL(pString == szNoBom);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SkipUtf16BomLE")
  {
    plUInt16 szWithBom[] = {0xfeff, 'a'};
    plUInt16 szNoBom[] = {'a'};

    const plUInt16* pString = szWithBom;

    PLASMA_TEST_BOOL(plUnicodeUtils::SkipUtf16BomLE(pString) == true);
    PLASMA_TEST_BOOL(pString == &szWithBom[1]);

    pString = szNoBom;

    PLASMA_TEST_BOOL(plUnicodeUtils::SkipUtf16BomLE(pString) == false);
    PLASMA_TEST_BOOL(pString == szNoBom);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SkipUtf16BomBE")
  {
    plUInt16 szWithBom[] = {0xfffe, 'a'};
    plUInt16 szNoBom[] = {'a'};

    const plUInt16* pString = szWithBom;

    PLASMA_TEST_BOOL(plUnicodeUtils::SkipUtf16BomBE(pString) == true);
    PLASMA_TEST_BOOL(pString == &szWithBom[1]);

    pString = szNoBom;

    PLASMA_TEST_BOOL(plUnicodeUtils::SkipUtf16BomBE(pString) == false);
    PLASMA_TEST_BOOL(pString == szNoBom);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsUtf16Surrogate")
  {
    plUInt16 szNoSurrogate[] = {0x2AD7};
    plUInt16 szSurrogate[] = {0xd83e};

    PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf16Surrogate(szNoSurrogate) == false);
    PLASMA_TEST_BOOL(plUnicodeUtils::IsUtf16Surrogate(szSurrogate) == true);
  }
}
