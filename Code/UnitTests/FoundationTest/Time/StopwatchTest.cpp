#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Stopwatch.h>

PLASMA_CREATE_SIMPLE_TEST(Time, Stopwatch)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "General Functionality")
  {
    plStopwatch sw;

    plThreadUtils::Sleep(plTime::Milliseconds(50));

    sw.StopAndReset();
    sw.Resume();

    const plTime t0 = sw.Checkpoint();

    plThreadUtils::Sleep(plTime::Milliseconds(10));

    const plTime t1 = sw.Checkpoint();

    plThreadUtils::Sleep(plTime::Milliseconds(20));

    const plTime t2 = sw.Checkpoint();

    plThreadUtils::Sleep(plTime::Milliseconds(30));

    const plTime t3 = sw.Checkpoint();

    const plTime tTotal1 = sw.GetRunningTotal();

    plThreadUtils::Sleep(plTime::Milliseconds(10));

    sw.Pause(); // freple the current running total

    const plTime tTotal2 = sw.GetRunningTotal();

    plThreadUtils::Sleep(plTime::Milliseconds(10)); // should not affect the running total anymore

    const plTime tTotal3 = sw.GetRunningTotal();


    // these tests are deliberately written such that they cannot fail,
    // even when the OS is under heavy load

    PLASMA_TEST_BOOL(t0 > plTime::Milliseconds(5));
    PLASMA_TEST_BOOL(t1 > plTime::Milliseconds(5));
    PLASMA_TEST_BOOL(t2 > plTime::Milliseconds(5));
    PLASMA_TEST_BOOL(t3 > plTime::Milliseconds(5));


    PLASMA_TEST_BOOL(t1 + t2 + t3 <= tTotal1);
    PLASMA_TEST_BOOL(t0 + t1 + t2 + t3 > tTotal1);

    PLASMA_TEST_BOOL(tTotal1 < tTotal2);
    PLASMA_TEST_BOOL(tTotal1 < tTotal3);
    PLASMA_TEST_BOOL(tTotal2 == tTotal3);
  }
}
