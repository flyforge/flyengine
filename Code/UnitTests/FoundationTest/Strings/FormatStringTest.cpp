#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#include <Foundation/Types/ScopeExit.h>
#include <stdarg.h>

void TestFormat(const plFormatString& str, const char* szExpected)
{
  plStringBuilder sb;
  plStringView szText = str.GetText(sb);

  PLASMA_TEST_STRING(szText, szExpected);
}

void TestFormatWChar(const plFormatString& str, const wchar_t* pExpected)
{
  plStringBuilder sb;
  plStringView szText = str.GetText(sb);

  PLASMA_TEST_WSTRING(plStringWChar(szText), pExpected);
}

void CompareSnprintf(plStringBuilder& ref_sLog, const plFormatString& str, const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char Temp1[256];
  char Temp2[256];

  // reusing args list crashes on GCC / Clang
  plStringUtils::vsnprintf(Temp1, 256, szFormat, args);
  vsnprintf(Temp2, 256, szFormat, args);
  PLASMA_TEST_STRING(Temp1, Temp2);

  plTime t1, t2, t3;
  plStopwatch sw;
  {
    sw.StopAndReset();

    for (plUInt32 i = 0; i < 10000; ++i)
    {
      plStringUtils::vsnprintf(Temp1, 256, szFormat, args);
    }

    t1 = sw.Checkpoint();
  }

  {
    sw.StopAndReset();

    for (plUInt32 i = 0; i < 10000; ++i)
    {
      vsnprintf(Temp2, 256, szFormat, args);
    }

    t2 = sw.Checkpoint();
  }

  {
    plStringBuilder sb;

    sw.StopAndReset();
    for (plUInt32 i = 0; i < 10000; ++i)
    {
      plStringView sText = str.GetText(sb);
    }

    t3 = sw.Checkpoint();
  }

  ref_sLog.AppendFormat("pl: {0} msec, std: {1} msec, plFmt: {2} msec : {3} -> {4}\n", plArgF(t1.GetMilliseconds(), 2), plArgF(t2.GetMilliseconds(), 2),
    plArgF(t3.GetMilliseconds(), 2), szFormat, Temp1);

  va_end(args);
}

PLASMA_CREATE_SIMPLE_TEST(Strings, FormatString)
{
  plStringBuilder perfLog;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basics")
  {
    const char* tmp = "stringviewstuff";

    const char* sz = "sz";
    plString string = "string";
    plStringBuilder sb = "builder";
    plStringView sv(tmp + 6, tmp + 10);

    TestFormat(plFmt("{0}, {1}, {2}, {3}", plInt8(-1), plInt16(-2), plInt32(-3), plInt64(-4)), "-1, -2, -3, -4");
    TestFormat(plFmt("{0}, {1}, {2}, {3}", plUInt8(1), plUInt16(2), plUInt32(3), plUInt64(4)), "1, 2, 3, 4");

    TestFormat(plFmt("{0}, {1}", plArgHumanReadable(0ll), plArgHumanReadable(1ll)), "0, 1");
    TestFormat(plFmt("{0}, {1}", plArgHumanReadable(-0ll), plArgHumanReadable(-1ll)), "0, -1");
    TestFormat(plFmt("{0}, {1}", plArgHumanReadable(999ll), plArgHumanReadable(1000ll)), "999, 1.00K");
    TestFormat(plFmt("{0}, {1}", plArgHumanReadable(-999ll), plArgHumanReadable(-1000ll)), "-999, -1.00K");
    // 999.999 gets rounded up for precision 2, so result is 1000.00K not 999.99K
    TestFormat(plFmt("{0}, {1}", plArgHumanReadable(999'999ll), plArgHumanReadable(1'000'000ll)), "1000.00K, 1.00M");
    TestFormat(plFmt("{0}, {1}", plArgHumanReadable(-999'999ll), plArgHumanReadable(-1'000'000ll)), "-1000.00K, -1.00M");

    TestFormat(plFmt("{0}, {1}", plArgFileSize(0u), plArgFileSize(1u)), "0B, 1B");
    TestFormat(plFmt("{0}, {1}", plArgFileSize(1023u), plArgFileSize(1024u)), "1023B, 1.00KB");
    // 1023.999 gets rounded up for precision 2, so result is 1024.00KB not 1023.99KB
    TestFormat(plFmt("{0}, {1}", plArgFileSize(1024u * 1024u - 1u), plArgFileSize(1024u * 1024u)), "1024.00KB, 1.00MB");

    const char* const suffixes[] = {" Foo", " Bar", " Foobar"};
    const plUInt32 suffixCount = PLASMA_ARRAY_SIZE(suffixes);
    TestFormat(plFmt("{0}", plArgHumanReadable(0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(plFmt("{0}", plArgHumanReadable(25ll, 25u, suffixes, suffixCount)), "1.00 Bar");
    TestFormat(plFmt("{0}", plArgHumanReadable(25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "2.00 Foobar");

    TestFormat(plFmt("{0}", plArgHumanReadable(-0ll, 25u, suffixes, suffixCount)), "0 Foo");
    TestFormat(plFmt("{0}", plArgHumanReadable(-25ll, 25u, suffixes, suffixCount)), "-1.00 Bar");
    TestFormat(plFmt("{0}", plArgHumanReadable(-25ll * 25ll * 2ll, 25u, suffixes, suffixCount)), "-2.00 Foobar");

    TestFormat(plFmt("'{0}, {1}'", "inl", sz), "'inl, sz'");
    TestFormat(plFmt("'{0}'", string), "'string'");
    TestFormat(plFmt("'{0}'", sb), "'builder'");
    TestFormat(plFmt("'{0}'", sv), "'view'");

    TestFormat(plFmt("{3}, {1}, {0}, {2}", plArgF(23.12345f, 1), plArgI(42), 17, 12.34f), "12.34, 42, 23.1, 17");

    const wchar_t* wsz = L"wsz";
    TestFormatWChar(plFmt("'{0}, {1}'", "inl", wsz), L"'inl, wsz'");
    TestFormatWChar(plFmt("'{0}, {1}'", L"inl", wsz), L"'inl, wsz'");
    // Temp buffer limit is 63 byte (64 including trailing zero). Each character in UTF-8 can potentially use 4 byte.
    // All input characters are 1 byte, so the 60th character is the last with 4 bytes left in the buffer.
    // Thus we end up with truncation after 60 characters.
    const wchar_t* wszTooLong = L"123456789.123456789.123456789.123456789.123456789.123456789.WAAAAAAAAAAAAAAH";
    const wchar_t* wszTooLongExpected = L"123456789.123456789.123456789.123456789.123456789.123456789.";
    const wchar_t* wszTooLongExpected2 =
      L"'123456789.123456789.123456789.123456789.123456789.123456789., 123456789.123456789.123456789.123456789.123456789.123456789.'";
    TestFormatWChar(plFmt("{0}", wszTooLong), wszTooLongExpected);
    TestFormatWChar(plFmt("'{0}, {1}'", wszTooLong, wszTooLong), wszTooLongExpected2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::DisabledNoWarning, "Compare Performance")
  {
    CompareSnprintf(perfLog, plFmt("Hello {0}, i = {1}, f = {2}", "World", 42, plArgF(3.141f, 2)), "Hello %s, i = %i, f = %.2f", "World", 42, 3.141f);
    CompareSnprintf(perfLog, plFmt("No formatting at all"), "No formatting at all");
    CompareSnprintf(perfLog, plFmt("{0}, {1}, {2}, {3}, {4}", "AAAAAA", "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE"), "%s, %s, %s, %s, %s", "AAAAAA",
      "BBBBBBB", "CCCCCC", "DDDDDDDDDDDDD", "EE");
    CompareSnprintf(perfLog, plFmt("{0}", 23), "%i", 23);
    CompareSnprintf(perfLog, plFmt("{0}", 23.123456789), "%f", 23.123456789);
    CompareSnprintf(perfLog, plFmt("{0}", plArgF(23.123456789, 2)), "%.2f", 23.123456789);
    CompareSnprintf(perfLog, plFmt("{0}", plArgI(123456789, 20, true)), "%020i", 123456789);
    CompareSnprintf(perfLog, plFmt("{0}", plArgI(123456789, 20, true, 16)), "%020X", 123456789);
    CompareSnprintf(perfLog, plFmt("{0}", plArgU(1234567890987ll, 30, false, 16)), "%30llx", 1234567890987ll);
    CompareSnprintf(perfLog, plFmt("{0}", plArgU(1234567890987ll, 30, false, 16, true)), "%30llX", 1234567890987ll);
    CompareSnprintf(perfLog, plFmt("{0}, {1}, {2}, {3}, {4}", 0, 1, 2, 3, 4), "%i, %i, %i, %i, %i", 0, 1, 2, 3, 4);
    CompareSnprintf(perfLog, plFmt("{0}, {1}, {2}, {3}, {4}", 0.1, 1.1, 2.1, 3.1, 4.1), "%.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1);
    CompareSnprintf(perfLog, plFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9),
      "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    CompareSnprintf(perfLog, plFmt("{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1),
      "%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f", 0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1);
    CompareSnprintf(perfLog, plFmt("{0}", plArgC('z')), "%c", 'z');

    CompareSnprintf(perfLog, plFmt("{}, {}, {}, {}, {}, {}, {}, {}, {}, {}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9), "%i, %i, %i, %i, %i, %i, %i, %i, %i, %i",
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9);

    // FILE* file = fopen("D:\\snprintf_perf.txt", "wb");
    // if (file)
    //{
    //  fwrite(perfLog.GetData(), 1, perfLog.GetElementCount(), file);
    //  fclose(file);
    //}
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Auto Increment")
  {
    TestFormat(plFmt("{}{}{}{}", plInt8(1), plInt16(2), plInt32(3), plInt64(4)), "1234");
    TestFormat(plFmt("{3}{2}{1}{0}", plInt8(1), plInt16(2), plInt32(3), plInt64(4)), "4321");

    TestFormat(plFmt("{}, {}, {}, {}", plInt8(-1), plInt16(-2), plInt32(-3), plInt64(-4)), "-1, -2, -3, -4");
    TestFormat(plFmt("{}, {}, {}, {}", plUInt8(1), plUInt16(2), plUInt32(3), plUInt64(4)), "1, 2, 3, 4");

    TestFormat(plFmt("{0}, {}, {}, {}", plUInt8(1), plUInt16(2), plUInt32(3), plUInt64(4)), "1, 2, 3, 4");

    TestFormat(plFmt("{1}, {}, {}, {}", plUInt8(1), plUInt16(2), plUInt32(3), plUInt64(4), plUInt64(5)), "2, 3, 4, 5");

    TestFormat(plFmt("{2}, {}, {1}, {}", plUInt8(1), plUInt16(2), plUInt32(3), plUInt64(4), plUInt64(5)), "3, 4, 2, 3");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plTime")
  {
    TestFormat(plFmt("{}", plTime()), "0ns");
    TestFormat(plFmt("{}", plTime::Nanoseconds(999)), "999ns");
    TestFormat(plFmt("{}", plTime::Nanoseconds(999.1)), "999.1ns");
    TestFormat(plFmt("{}", plTime::Microseconds(999)), u8"999\u00B5s");     // Utf-8 encoding for the microsecond sign
    TestFormat(plFmt("{}", plTime::Microseconds(999.2)), u8"999.2\u00B5s"); // Utf-8 encoding for the microsecond sign
    TestFormat(plFmt("{}", plTime::Milliseconds(-999)), "-999ms");
    TestFormat(plFmt("{}", plTime::Milliseconds(-999.3)), "-999.3ms");
    TestFormat(plFmt("{}", plTime::Seconds(59)), "59sec");
    TestFormat(plFmt("{}", plTime::Seconds(-59.9)), "-59.9sec");
    TestFormat(plFmt("{}", plTime::Seconds(75)), "1min 15sec");
    TestFormat(plFmt("{}", plTime::Seconds(-75.4)), "-1min 15.4sec");
    TestFormat(plFmt("{}", plTime::Minutes(59)), "59min 0sec");
    TestFormat(plFmt("{}", plTime::Minutes(-1)), "-1min 0sec");
    TestFormat(plFmt("{}", plTime::Minutes(90)), "1h 30min 0sec");
    TestFormat(plFmt("{}", plTime::Minutes(-90.5)), "-1h 30min 30sec");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plDateTime")
  {
    {
      plDateTime dt;
      dt.SetYear(2019);
      dt.SetMonth(6);
      dt.SetDay(12);
      dt.SetHour(13);
      dt.SetMinute(26);
      dt.SetSecond(51);
      dt.SetMicroseconds(7000);

      TestFormat(plFmt("{}", dt), "2019-06-12_13-26-51-007");
    }

    {
      plDateTime dt;
      dt.SetYear(0);
      dt.SetMonth(1);
      dt.SetDay(1);
      dt.SetHour(0);
      dt.SetMinute(0);
      dt.SetSecond(0);
      dt.SetMicroseconds(0);

      TestFormat(plFmt("{}", dt), "0000-01-01_00-00-00-000");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sensitive Info")
  {
    auto prev = plArgSensitive::s_BuildStringCB;
    PLASMA_SCOPE_EXIT(plArgSensitive::s_BuildStringCB = prev);

    plArgSensitive::s_BuildStringCB = plArgSensitive::BuildString_SensitiveUserData_Hash;

    plStringBuilder fmt;

    fmt.Format("Password: {}", plArgSensitive("hunter2", "pwd"));
    PLASMA_TEST_STRING(fmt, "Password: sud:pwd#96d66ce6($7)");

    fmt.Format("Password: {}", plArgSensitive("hunter2"));
    PLASMA_TEST_STRING(fmt, "Password: sud:#96d66ce6($7)");
  }
}
