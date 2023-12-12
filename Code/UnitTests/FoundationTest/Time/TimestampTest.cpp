#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

PLASMA_CREATE_SIMPLE_TEST(Time, Timestamp)
{
  const plInt64 iFirstContactUnixTimeInSeconds = 2942956800LL;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructors / Valid Check")
  {
    plTimestamp invalidTimestamp;
    PLASMA_TEST_BOOL(!invalidTimestamp.IsValid());

    plTimestamp validTimestamp(0, plSIUnitOfTime::Second);
    PLASMA_TEST_BOOL(validTimestamp.IsValid());
    validTimestamp.Invalidate();
    PLASMA_TEST_BOOL(!validTimestamp.IsValid());

    plTimestamp currentTimestamp = plTimestamp::CurrentTimestamp();
    // Kind of hard to hit a moving target, let's just test if it is in a probable range.
    PLASMA_TEST_BOOL(currentTimestamp.IsValid());
    PLASMA_TEST_BOOL_MSG(currentTimestamp.GetInt64(plSIUnitOfTime::Second) > 1384597970LL, "The current time is before this test was written!");
    PLASMA_TEST_BOOL_MSG(currentTimestamp.GetInt64(plSIUnitOfTime::Second) < 32531209845LL,
      "This current time is after the year 3000! If this is actually the case, please fix this test.");

    // Sleep for 10 milliseconds
    plThreadUtils::Sleep(plTime::Milliseconds(10));
    PLASMA_TEST_BOOL_MSG(currentTimestamp.GetInt64(plSIUnitOfTime::Microsecond) < plTimestamp::CurrentTimestamp().GetInt64(plSIUnitOfTime::Microsecond),
      "Sleeping for 10 ms should cause the timestamp to change!");
    PLASMA_TEST_BOOL_MSG(!currentTimestamp.Compare(plTimestamp::CurrentTimestamp(), plTimestamp::CompareMode::Identical),
      "Sleeping for 10 ms should cause the timestamp to change!");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Public Accessors")
  {
    const plTimestamp epoch(0, plSIUnitOfTime::Second);
    const plTimestamp firstContact(iFirstContactUnixTimeInSeconds, plSIUnitOfTime::Second);
    PLASMA_TEST_BOOL(epoch.IsValid());
    PLASMA_TEST_BOOL(firstContact.IsValid());

    // GetInt64 / SetInt64
    plTimestamp firstContactTest(iFirstContactUnixTimeInSeconds, plSIUnitOfTime::Second);
    PLASMA_TEST_INT(firstContactTest.GetInt64(plSIUnitOfTime::Second), iFirstContactUnixTimeInSeconds);
    PLASMA_TEST_INT(firstContactTest.GetInt64(plSIUnitOfTime::Millisecond), iFirstContactUnixTimeInSeconds * 1000LL);
    PLASMA_TEST_INT(firstContactTest.GetInt64(plSIUnitOfTime::Microsecond), iFirstContactUnixTimeInSeconds * 1000000LL);
    PLASMA_TEST_INT(firstContactTest.GetInt64(plSIUnitOfTime::Nanosecond), iFirstContactUnixTimeInSeconds * 1000000000LL);

    firstContactTest.SetInt64(firstContactTest.GetInt64(plSIUnitOfTime::Second), plSIUnitOfTime::Second);
    PLASMA_TEST_BOOL(firstContactTest.Compare(firstContact, plTimestamp::CompareMode::Identical));
    firstContactTest.SetInt64(firstContactTest.GetInt64(plSIUnitOfTime::Millisecond), plSIUnitOfTime::Millisecond);
    PLASMA_TEST_BOOL(firstContactTest.Compare(firstContact, plTimestamp::CompareMode::Identical));
    firstContactTest.SetInt64(firstContactTest.GetInt64(plSIUnitOfTime::Microsecond), plSIUnitOfTime::Microsecond);
    PLASMA_TEST_BOOL(firstContactTest.Compare(firstContact, plTimestamp::CompareMode::Identical));
    firstContactTest.SetInt64(firstContactTest.GetInt64(plSIUnitOfTime::Nanosecond), plSIUnitOfTime::Nanosecond);
    PLASMA_TEST_BOOL(firstContactTest.Compare(firstContact, plTimestamp::CompareMode::Identical));

    // IsEqual
    const plTimestamp firstContactPlusAFewMicroseconds(firstContact.GetInt64(plSIUnitOfTime::Microsecond) + 42, plSIUnitOfTime::Microsecond);
    PLASMA_TEST_BOOL(firstContact.Compare(firstContactPlusAFewMicroseconds, plTimestamp::CompareMode::FileTimeEqual));
    PLASMA_TEST_BOOL(!firstContact.Compare(firstContactPlusAFewMicroseconds, plTimestamp::CompareMode::Identical));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operators")
  {
    const plTimestamp firstContact(iFirstContactUnixTimeInSeconds, plSIUnitOfTime::Second);

    // Time span arithmetics
    const plTime timeSpan1000s = plTime::Seconds(1000);
    PLASMA_TEST_BOOL(timeSpan1000s.GetMicroseconds() == 1000000000LL);

    // operator +
    const plTimestamp firstContactPlus1000s = firstContact + timeSpan1000s;
    plInt64 iSpanDiff = firstContactPlus1000s.GetInt64(plSIUnitOfTime::Microsecond) - firstContact.GetInt64(plSIUnitOfTime::Microsecond);
    PLASMA_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    PLASMA_TEST_BOOL(firstContactPlus1000s - firstContact == timeSpan1000s);

    const plTimestamp T1000sPlusFirstContact = timeSpan1000s + firstContact;
    iSpanDiff = T1000sPlusFirstContact.GetInt64(plSIUnitOfTime::Microsecond) - firstContact.GetInt64(plSIUnitOfTime::Microsecond);
    PLASMA_TEST_BOOL(iSpanDiff == 1000000000LL);
    // You can only subtract points in time
    PLASMA_TEST_BOOL(T1000sPlusFirstContact - firstContact == timeSpan1000s);

    // operator -
    const plTimestamp firstContactMinus1000s = firstContact - timeSpan1000s;
    iSpanDiff = firstContactMinus1000s.GetInt64(plSIUnitOfTime::Microsecond) - firstContact.GetInt64(plSIUnitOfTime::Microsecond);
    PLASMA_TEST_BOOL(iSpanDiff == -1000000000LL);
    // You can only subtract points in time
    PLASMA_TEST_BOOL(firstContact - firstContactMinus1000s == timeSpan1000s);


    // operator += / -=
    plTimestamp testTimestamp = firstContact;
    testTimestamp += timeSpan1000s;
    PLASMA_TEST_BOOL(testTimestamp.Compare(firstContactPlus1000s, plTimestamp::CompareMode::Identical));
    testTimestamp -= timeSpan1000s;
    PLASMA_TEST_BOOL(testTimestamp.Compare(firstContact, plTimestamp::CompareMode::Identical));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plDateTime conversion")
  {
    // Constructor
    plDateTime invalidDateTime;
    PLASMA_TEST_BOOL(!invalidDateTime.GetTimestamp().IsValid());

    const plTimestamp firstContact(iFirstContactUnixTimeInSeconds, plSIUnitOfTime::Second);
    plDateTime firstContactDataTime(firstContact);

    // Getter
    PLASMA_TEST_INT(firstContactDataTime.GetYear(), 2063);
    PLASMA_TEST_INT(firstContactDataTime.GetMonth(), 4);
    PLASMA_TEST_INT(firstContactDataTime.GetDay(), 5);
    PLASMA_TEST_BOOL(firstContactDataTime.GetDayOfWeek() == 4 ||
                 firstContactDataTime.GetDayOfWeek() == 255); // not supported on all platforms, should output 255 then
    PLASMA_TEST_INT(firstContactDataTime.GetHour(), 0);
    PLASMA_TEST_INT(firstContactDataTime.GetMinute(), 0);
    PLASMA_TEST_INT(firstContactDataTime.GetSecond(), 0);
    PLASMA_TEST_INT(firstContactDataTime.GetMicroseconds(), 0);

    // SetTimestamp / GetTimestamp
    plTimestamp currentTimestamp = plTimestamp::CurrentTimestamp();
    plDateTime currentDateTime;
    currentDateTime.SetTimestamp(currentTimestamp);
    plTimestamp currentTimestamp2 = currentDateTime.GetTimestamp();
    // OS date time functions should be accurate within one second.
    plInt64 iDiff = plMath::Abs(currentTimestamp.GetInt64(plSIUnitOfTime::Microsecond) - currentTimestamp2.GetInt64(plSIUnitOfTime::Microsecond));
    PLASMA_TEST_BOOL(iDiff <= 1000000);

    // Setter
    plDateTime oneSmallStep;
    oneSmallStep.SetYear(1969);
    oneSmallStep.SetMonth(7);
    oneSmallStep.SetDay(21);
    oneSmallStep.SetDayOfWeek(1);
    oneSmallStep.SetHour(2);
    oneSmallStep.SetMinute(56);
    oneSmallStep.SetSecond(0);
    oneSmallStep.SetMicroseconds(0);

    plTimestamp oneSmallStepTimestamp = oneSmallStep.GetTimestamp();
    PLASMA_TEST_BOOL(oneSmallStepTimestamp.IsValid());
    PLASMA_TEST_INT(oneSmallStepTimestamp.GetInt64(plSIUnitOfTime::Second), -14159040LL);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plDateTime formatting")
  {
    plDateTime dateTime;

    dateTime.SetYear(2019);
    dateTime.SetMonth(8);
    dateTime.SetDay(16);
    dateTime.SetDayOfWeek(5);
    dateTime.SetHour(13);
    dateTime.SetMinute(40);
    dateTime.SetSecond(30);
    dateTime.SetMicroseconds(345678);

    char szTimestampFormatted[256] = "";

    // no names, no UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::Default));
    PLASMA_TEST_STRING("2019-08-16 - 13:40:30", szTimestampFormatted);
    // no names, no UTC, with milliseconds
    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::Default | plArgDateTime::ShowMilliseconds));
    PLASMA_TEST_STRING("2019-08-16 - 13:40:30.345", szTimestampFormatted);
    // no names, with UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::Default | plArgDateTime::ShowTimeZone));
    PLASMA_TEST_STRING("2019-08-16 - 13:40:30 (UTC)", szTimestampFormatted);
    // no names, with UTC, with milliseconds
    BuildString(
      szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::ShowDate | plArgDateTime::ShowMilliseconds | plArgDateTime::ShowTimeZone));
    PLASMA_TEST_STRING("2019-08-16 - 13:40:30.345 (UTC)", szTimestampFormatted);
    // with names, no UTC, no milliseconds
    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::DefaultTextual | plArgDateTime::ShowWeekday));
    PLASMA_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30", szTimestampFormatted);
    // no names, no UTC, with milliseconds
    BuildString(szTimestampFormatted, 256,
      plArgDateTime(dateTime, plArgDateTime::DefaultTextual | plArgDateTime::ShowWeekday | plArgDateTime::ShowMilliseconds));
    PLASMA_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30.345", szTimestampFormatted);
    // no names, with UTC, no milliseconds
    BuildString(
      szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::DefaultTextual | plArgDateTime::ShowWeekday | plArgDateTime::ShowTimeZone));
    PLASMA_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30 (UTC)", szTimestampFormatted);
    // no names, with UTC, with milliseconds
    BuildString(szTimestampFormatted, 256,
      plArgDateTime(
        dateTime, plArgDateTime::DefaultTextual | plArgDateTime::ShowWeekday | plArgDateTime::ShowMilliseconds | plArgDateTime::ShowTimeZone));
    PLASMA_TEST_STRING("2019 Aug 16 (Fri) - 13:40:30.345 (UTC)", szTimestampFormatted);

    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::ShowDate));
    PLASMA_TEST_STRING("2019-08-16", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::TextualDate));
    PLASMA_TEST_STRING("2019 Aug 16", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::ShowTime));
    PLASMA_TEST_STRING("13:40", szTimestampFormatted);
    BuildString(szTimestampFormatted, 256, plArgDateTime(dateTime, plArgDateTime::ShowWeekday | plArgDateTime::ShowMilliseconds));
    PLASMA_TEST_STRING("(Fri) - 13:40:30.345", szTimestampFormatted);
  }
}
