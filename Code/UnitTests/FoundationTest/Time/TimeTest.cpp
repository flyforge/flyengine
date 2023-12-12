#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Time/Time.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Time);

PLASMA_CREATE_SIMPLE_TEST(Time, Timer)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basics")
  {
    plTime TestTime = plTime::Now();

    PLASMA_TEST_BOOL(TestTime.GetMicroseconds() > 0.0);

    volatile plUInt32 testValue = 0;
    for (plUInt32 i = 0; i < 42000; ++i)
    {
      testValue += 23;
    }

    plTime TestTime2 = plTime::Now();

    PLASMA_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);

    TestTime2 -= TestTime;

    PLASMA_TEST_BOOL(TestTime2.GetMicroseconds() > 0.0);
  }
}
