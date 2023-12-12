#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/ConversionUtils.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Utility);

PLASMA_CREATE_SIMPLE_TEST(Utility, ConversionUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StringToInt")
  {
    const char* szString = "1a";
    const char* szResultPos = nullptr;

    plInt32 iRes = 42;
    szString = "01234";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 1234);
    PLASMA_TEST_BOOL(szResultPos == szString + 5);

    iRes = 42;
    szString = "0";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 0);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0000";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 0);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, -999999);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-+999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, -999999);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "--999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 999999);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "++---+--+--999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, -999999);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "++--+--+--999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 999999);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "123+456";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 123);
    PLASMA_TEST_BOOL(szResultPos == szString + 3);

    iRes = 42;
    szString = "123_456";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 123);
    PLASMA_TEST_BOOL(szResultPos == szString + 3);

    iRes = 42;
    szString = "-123-456";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, -123);
    PLASMA_TEST_BOOL(szResultPos == szString + 4);


    iRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(nullptr, iRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt("", iRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt("a", iRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt("a15", iRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt("+", iRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt("-", iRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "1a";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 1);
    PLASMA_TEST_BOOL(szResultPos == szString + 1);

    iRes = 42;
    szString = "0 23";
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 0);
    PLASMA_TEST_BOOL(szResultPos == szString + 1);

    // overflow check

    iRes = 42;
    szString = "0002147483647"; // valid
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 2147483647);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-2147483648"; // valid
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, (plInt32)0x80000000);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0002147483648"; // invalid
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "-2147483649"; // invalid
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt(szString, iRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StringToUInt")
  {
    const char* szString = "1a";
    const char* szResultPos = nullptr;

    plUInt32 uiRes = 42;
    szString = "01234";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 1234);
    PLASMA_TEST_BOOL(szResultPos == szString + 5);

    uiRes = 42;
    szString = "0";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 0);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "0000";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 0);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "-999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "-+999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "--999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 999999);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "++---+--+--999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "++--+--+--999999";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 999999);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "123+456";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 123);
    PLASMA_TEST_BOOL(szResultPos == szString + 3);

    uiRes = 42;
    szString = "123_456";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 123);
    PLASMA_TEST_BOOL(szResultPos == szString + 3);

    uiRes = 42;
    szString = "-123-456";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);
    PLASMA_TEST_BOOL(szResultPos == szString + 4);


    uiRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(nullptr, uiRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);

    uiRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt("", uiRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);

    uiRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt("a", uiRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);

    uiRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt("a15", uiRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);

    uiRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt("+", uiRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);

    uiRes = 42;
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt("-", uiRes) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);

    uiRes = 42;
    szString = "1a";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 1);
    PLASMA_TEST_BOOL(szResultPos == szString + 1);

    uiRes = 42;
    szString = "0 23";
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 0);
    PLASMA_TEST_BOOL(szResultPos == szString + 1);

    // overflow check

    uiRes = 42;
    szString = "0004294967295"; // valid
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(uiRes, 4294967295u);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    uiRes = 42;
    szString = "0004294967296"; // invalid
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);

    uiRes = 42;
    szString = "-1"; // invalid
    PLASMA_TEST_BOOL(plConversionUtils::StringToUInt(szString, uiRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(uiRes, 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StringToInt64")
  {
    // overflow check
    plInt64 iRes = 42;
    const char* szString = "0002147483639"; // valid
    const char* szResultPos = nullptr;

    PLASMA_TEST_BOOL(plConversionUtils::StringToInt64(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 2147483639);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0002147483640"; // also valid with 64bit
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt64(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 2147483640);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0009223372036854775807"; // last valid positive number
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt64(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, 9223372036854775807);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "0009223372036854775808"; // invalid
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt64(szString, iRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);

    iRes = 42;
    szString = "-9223372036854775808"; // last valid negative number
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt64(szString, iRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(iRes, (plInt64)0x8000000000000000);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    iRes = 42;
    szString = "-9223372036854775809"; // invalid
    PLASMA_TEST_BOOL(plConversionUtils::StringToInt64(szString, iRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_INT(iRes, 42);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StringToFloat")
  {
    const char* szString = nullptr;
    const char* szResultPos = nullptr;

    double fRes = 42;
    szString = "23.45";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 23.45, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "-2345";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, -2345.0, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "-0";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 0.0, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "0_0000.0_00000_";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 0.0, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "_0_0000.0_00000_";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_FAILURE);

    fRes = 42;
    szString = ".123456789";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 0.123456789, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "+123E1";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 1230.0, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "  \r\t 123e0";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 123.0, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "\n123e6";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 123000000.0, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "\n1_2_3e+6";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 123000000.0, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = "  123E-6";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 0.000123, 0.00001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = " + - -+-123.45e-10";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, -0.000000012345, 0.0000001);
    PLASMA_TEST_BOOL(szResultPos == szString + plStringUtils::GetStringElementCount(szString));

    fRes = 42;
    szString = nullptr;
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString = "";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString = "-----";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_DOUBLE(fRes, 42.0, 0.00001);

    fRes = 42;
    szString = " + - +++ - \r \n";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_DOUBLE(fRes, 42.0, 0.00001);


    fRes = 42;
    szString = "65.345789xabc";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, 65.345789, 0.000001);
    PLASMA_TEST_BOOL(szResultPos == szString + 9);

    fRes = 42;
    szString = " \n \r \t + - 2314565.345789ff xabc";
    PLASMA_TEST_BOOL(plConversionUtils::StringToFloat(szString, fRes, &szResultPos) == PLASMA_SUCCESS);
    PLASMA_TEST_DOUBLE(fRes, -2314565.345789, 0.000001);
    PLASMA_TEST_BOOL(szResultPos == szString + 25);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StringToBool")
  {
    const char* szString = "";
    const char* szResultPos = nullptr;
    bool bRes = false;

    // true / false
    {
      bRes = false;
      szString = "true,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "FALSe,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(!bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');
    }

    // on / off
    {
      bRes = false;
      szString = "\n on,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "\t\t \toFf,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(!bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');
    }

    // 1 / 0
    {
      bRes = false;
      szString = "\r1,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "0,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(!bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');
    }

    // yes / no
    {
      bRes = false;
      szString = "yes,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "NO,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(!bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');
    }

    // enable / disable
    {
      bRes = false;
      szString = "enable,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');

      bRes = true;
      szString = "disABle,";
      szResultPos = nullptr;
      PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_SUCCESS);
      PLASMA_TEST_BOOL(!bRes);
      PLASMA_TEST_BOOL(*szResultPos == ',');
    }

    bRes = false;

    szString = "of,";
    szResultPos = nullptr;
    PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(szResultPos == nullptr);

    szString = "aon";
    szResultPos = nullptr;
    PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(szResultPos == nullptr);

    szString = "";
    szResultPos = nullptr;
    PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(szResultPos == nullptr);

    szString = nullptr;
    szResultPos = nullptr;
    PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(szResultPos == nullptr);

    szString = "tut";
    szResultPos = nullptr;
    PLASMA_TEST_BOOL(plConversionUtils::StringToBool(szString, bRes, &szResultPos) == PLASMA_FAILURE);
    PLASMA_TEST_BOOL(szResultPos == nullptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HexCharacterToIntValue")
  {
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('0'), 0);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('1'), 1);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('2'), 2);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('3'), 3);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('4'), 4);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('5'), 5);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('6'), 6);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('7'), 7);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('8'), 8);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('9'), 9);

    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('a'), 10);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('b'), 11);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('c'), 12);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('d'), 13);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('e'), 14);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('f'), 15);

    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('A'), 10);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('B'), 11);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('C'), 12);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('D'), 13);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('E'), 14);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('F'), 15);

    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('g'), -1);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('h'), -1);
    PLASMA_TEST_INT(plConversionUtils::HexCharacterToIntValue('i'), -1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ConvertHexStringToUInt32")
  {
    plUInt32 res;

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("0x", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("0", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("0x0", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("a", res).Succeeded());
    PLASMA_TEST_BOOL(res == 10);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("0xb", res).Succeeded());
    PLASMA_TEST_BOOL(res == 11);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("000c", res).Succeeded());
    PLASMA_TEST_BOOL(res == 12);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("AA", res).Succeeded());
    PLASMA_TEST_BOOL(res == 170);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("aAjbB", res).Failed());

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("aAbB", res).Succeeded());
    PLASMA_TEST_BOOL(res == 43707);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("FFFFffff", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFFF);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("0000FFFFffff", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFF);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt32("100000000", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0x10000000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ConvertHexStringToUInt64")
  {
    plUInt64 res;

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("0x", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("0", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("0x0", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("a", res).Succeeded());
    PLASMA_TEST_BOOL(res == 10);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("0xb", res).Succeeded());
    PLASMA_TEST_BOOL(res == 11);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("000c", res).Succeeded());
    PLASMA_TEST_BOOL(res == 12);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("AA", res).Succeeded());
    PLASMA_TEST_BOOL(res == 170);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("aAjbB", res).Failed());

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("aAbB", res).Succeeded());
    PLASMA_TEST_BOOL(res == 43707);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("FFFFffff", res).Succeeded());
    PLASMA_TEST_BOOL(res == 4294967295);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("0000FFFFffff", res).Succeeded());
    PLASMA_TEST_BOOL(res == 4294967295);

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("0xfffffffffffffffy", res).Failed());

    PLASMA_TEST_BOOL(plConversionUtils::ConvertHexStringToUInt64("0xffffffffffffffffy", res).Succeeded());
    PLASMA_TEST_BOOL(res == 0xFFFFFFFFFFFFFFFFllu);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ConvertBinaryToHex and ConvertHexStringToBinary")
  {
    plDynamicArray<plUInt8> binary;
    binary.SetCountUninitialized(1024);

    plRandom r;
    r.InitializeFromCurrentTime();

    for (auto& val : binary)
    {
      val = static_cast<plUInt8>(r.UIntInRange(256u));
    }

    plStringBuilder sHex;
    plConversionUtils::ConvertBinaryToHex(binary.GetData(), binary.GetCount(), [&sHex](const char* s) { sHex.Append(s); });

    plDynamicArray<plUInt8> binary2;
    binary2.SetCountUninitialized(1024);

    plConversionUtils::ConvertHexToBinary(sHex, binary2.GetData(), binary2.GetCount());

    PLASMA_TEST_BOOL(binary == binary2);
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExtractFloatsFromString")
  {
    float v[16];

    const char* szText = "This 1 is 2.3 or 3.141 tests in 1.2 strings, maybe 4.5,6.78or9.101!";

    plMemoryUtils::ZeroFill(v, 16);
    PLASMA_TEST_INT(plConversionUtils::ExtractFloatsFromString(szText, 0, v), 0);
    PLASMA_TEST_FLOAT(v[0], 0.0f, 0.0f);

    plMemoryUtils::ZeroFill(v, 16);
    PLASMA_TEST_INT(plConversionUtils::ExtractFloatsFromString(szText, 3, v), 3);
    PLASMA_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    PLASMA_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    PLASMA_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    PLASMA_TEST_FLOAT(v[3], 0.0f, 0.0f);

    plMemoryUtils::ZeroFill(v, 16);
    PLASMA_TEST_INT(plConversionUtils::ExtractFloatsFromString(szText, 6, v), 6);
    PLASMA_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    PLASMA_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    PLASMA_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    PLASMA_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    PLASMA_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    PLASMA_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    PLASMA_TEST_FLOAT(v[6], 0.0f, 0.0f);

    plMemoryUtils::ZeroFill(v, 16);
    PLASMA_TEST_INT(plConversionUtils::ExtractFloatsFromString(szText, 10, v), 7);
    PLASMA_TEST_FLOAT(v[0], 1.0f, 0.0001f);
    PLASMA_TEST_FLOAT(v[1], 2.3f, 0.0001f);
    PLASMA_TEST_FLOAT(v[2], 3.141f, 0.0001f);
    PLASMA_TEST_FLOAT(v[3], 1.2f, 0.0001f);
    PLASMA_TEST_FLOAT(v[4], 4.5f, 0.0001f);
    PLASMA_TEST_FLOAT(v[5], 6.78f, 0.0001f);
    PLASMA_TEST_FLOAT(v[6], 9.101f, 0.0001f);
    PLASMA_TEST_FLOAT(v[7], 0.0f, 0.0f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ConvertStringToUuid and IsStringUuid")
  {
    plUuid guid;
    plStringBuilder sGuid;

    for (plUInt32 i = 0; i < 100; ++i)
    {
      guid.CreateNewUuid();

      plConversionUtils::ToString(guid, sGuid);

      PLASMA_TEST_BOOL(plConversionUtils::IsStringUuid(sGuid));

      plUuid guid2 = plConversionUtils::ConvertStringToUuid(sGuid);

      PLASMA_TEST_BOOL(guid == guid2);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetColorName")
  {
    PLASMA_TEST_STRING(plString(plConversionUtils::GetColorName(plColorGammaUB(1, 2, 3))), "#010203");
    PLASMA_TEST_STRING(plString(plConversionUtils::GetColorName(plColorGammaUB(10, 20, 30, 40))), "#0A141E28");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetColorByName")
  {
    PLASMA_TEST_BOOL(plConversionUtils::GetColorByName("#010203") == plColorGammaUB(1, 2, 3));
    PLASMA_TEST_BOOL(plConversionUtils::GetColorByName("#0A141E28") == plColorGammaUB(10, 20, 30, 40));

    PLASMA_TEST_BOOL(plConversionUtils::GetColorByName("#010203") == plColorGammaUB(1, 2, 3));
    PLASMA_TEST_BOOL(plConversionUtils::GetColorByName("#0a141e28") == plColorGammaUB(10, 20, 30, 40));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetColorByName and GetColorName")
  {
#define Check(name)                                                                  \
  {                                                                                  \
    bool valid = false;                                                              \
    const plColor c = plConversionUtils::GetColorByName(PLASMA_STRINGIZE(name), &valid); \
    PLASMA_TEST_BOOL(valid);                                                             \
    plString sName = plConversionUtils::GetColorName(c);                             \
    PLASMA_TEST_STRING(sName, PLASMA_STRINGIZE(name));                                       \
  }

#define Check2(name, otherName)                                                      \
  {                                                                                  \
    bool valid = false;                                                              \
    const plColor c = plConversionUtils::GetColorByName(PLASMA_STRINGIZE(name), &valid); \
    PLASMA_TEST_BOOL(valid);                                                             \
    plString sName = plConversionUtils::GetColorName(c);                             \
    PLASMA_TEST_STRING(sName, PLASMA_STRINGIZE(otherName));                                  \
  }

    Check(AliceBlue);
    Check(AntiqueWhite);
    Check(Aqua);
    Check(Aquamarine);
    Check(Azure);
    Check(Beige);
    Check(Bisque);
    Check(Black);
    Check(BlanchedAlmond);
    Check(Blue);
    Check(BlueViolet);
    Check(Brown);
    Check(BurlyWood);
    Check(CadetBlue);
    Check(Chartreuse);
    Check(Chocolate);
    Check(Coral);
    Check(CornflowerBlue); // The Original!
    Check(Cornsilk);
    Check(Crimson);
    Check2(Cyan, Aqua);
    Check(DarkBlue);
    Check(DarkCyan);
    Check(DarkGoldenRod);
    Check(DarkGray);
    Check2(DarkGrey, DarkGray);
    Check(DarkGreen);
    Check(DarkKhaki);
    Check(DarkMagenta);
    Check(DarkOliveGreen);
    Check(DarkOrange);
    Check(DarkOrchid);
    Check(DarkRed);
    Check(DarkSalmon);
    Check(DarkSeaGreen);
    Check(DarkSlateBlue);
    Check(DarkSlateGray);
    Check2(DarkSlateGrey, DarkSlateGray);
    Check(DarkTurquoise);
    Check(DarkViolet);
    Check(DeepPink);
    Check(DeepSkyBlue);
    Check(DimGray);
    Check2(DimGrey, DimGray);
    Check(DodgerBlue);
    Check(FireBrick);
    Check(FloralWhite);
    Check(ForestGreen);
    Check(Fuchsia);
    Check(Gainsboro);
    Check(GhostWhite);
    Check(Gold);
    Check(GoldenRod);
    Check(Gray);
    Check2(Grey, Gray);
    Check(Green);
    Check(GreenYellow);
    Check(HoneyDew);
    Check(HotPink);
    Check(IndianRed);
    Check(Indigo);
    Check(Ivory);
    Check(Khaki);
    Check(Lavender);
    Check(LavenderBlush);
    Check(LawnGreen);
    Check(LemonChiffon);
    Check(LightBlue);
    Check(LightCoral);
    Check(LightCyan);
    Check(LightGoldenRodYellow);
    Check(LightGray);
    Check2(LightGrey, LightGray);
    Check(LightGreen);
    Check(LightPink);
    Check(LightSalmon);
    Check(LightSeaGreen);
    Check(LightSkyBlue);
    Check(LightSlateGray);
    Check2(LightSlateGrey, LightSlateGray);
    Check(LightSteelBlue);
    Check(LightYellow);
    Check(Lime);
    Check(LimeGreen);
    Check(Linen);
    Check2(Magenta, Fuchsia);
    Check(Maroon);
    Check(MediumAquaMarine);
    Check(MediumBlue);
    Check(MediumOrchid);
    Check(MediumPurple);
    Check(MediumSeaGreen);
    Check(MediumSlateBlue);
    Check(MediumSpringGreen);
    Check(MediumTurquoise);
    Check(MediumVioletRed);
    Check(MidnightBlue);
    Check(MintCream);
    Check(MistyRose);
    Check(Moccasin);
    Check(NavajoWhite);
    Check(Navy);
    Check(OldLace);
    Check(Olive);
    Check(OliveDrab);
    Check(Orange);
    Check(OrangeRed);
    Check(Orchid);
    Check(PaleGoldenRod);
    Check(PaleGreen);
    Check(PaleTurquoise);
    Check(PaleVioletRed);
    Check(PapayaWhip);
    Check(PeachPuff);
    Check(Peru);
    Check(Pink);
    Check(Plum);
    Check(PowderBlue);
    Check(Purple);
    Check(RebeccaPurple);
    Check(Red);
    Check(RosyBrown);
    Check(RoyalBlue);
    Check(SaddleBrown);
    Check(Salmon);
    Check(SandyBrown);
    Check(SeaGreen);
    Check(SeaShell);
    Check(Sienna);
    Check(Silver);
    Check(SkyBlue);
    Check(SlateBlue);
    Check(SlateGray);
    Check2(SlateGrey, SlateGray);
    Check(Snow);
    Check(SpringGreen);
    Check(SteelBlue);
    Check(Tan);
    Check(Teal);
    Check(Thistle);
    Check(Tomato);
    Check(Turquoise);
    Check(Violet);
    Check(Wheat);
    Check(White);
    Check(WhiteSmoke);
    Check(Yellow);
    Check(YellowGreen);
  }
}
