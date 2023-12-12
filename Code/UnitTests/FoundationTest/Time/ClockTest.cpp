#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Clock.h>

class plSimpleTimeStepSmoother : public plTimeStepSmoothing
{
public:
  virtual plTime GetSmoothedTimeStep(plTime rawTimeStep, const plClock* pClock) override { return plTime::Seconds(0.42); }

  virtual void Reset(const plClock* pClock) override {}
};

PLASMA_CREATE_SIMPLE_TEST(Time, Clock)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor / Reset")
  {
    plClock c("Test"); // calls 'Reset' internally

    PLASMA_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor

    PLASMA_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 0.0, 0.0);
    PLASMA_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);
    PLASMA_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);
    PLASMA_TEST_BOOL(c.GetPaused() == false);
    PLASMA_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants
    PLASMA_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0);   // to ensure the tests fail if somebody changes these constants
    PLASMA_TEST_BOOL(c.GetTimeDiff() > plTime::Seconds(0.0));

    plSimpleTimeStepSmoother s;

    c.SetTimeStepSmoothing(&s);

    PLASMA_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(false);

    // does NOT reset which time step smoother to use
    PLASMA_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.Reset(true);
    PLASMA_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr); // after constructor
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetPaused / GetPaused")
  {
    plClock c("Test");
    PLASMA_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    PLASMA_TEST_BOOL(c.GetPaused());

    c.SetPaused(false);
    PLASMA_TEST_BOOL(!c.GetPaused());

    c.SetPaused(true);
    PLASMA_TEST_BOOL(c.GetPaused());

    c.Reset(false);
    PLASMA_TEST_BOOL(!c.GetPaused());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Updates while Paused / Unpaused")
  {
    plClock c("Test");

    c.SetPaused(false);

    const plTime t0 = c.GetAccumulatedTime();

    plThreadUtils::Sleep(plTime::Milliseconds(10));
    c.Update();

    const plTime t1 = c.GetAccumulatedTime();
    PLASMA_TEST_BOOL(t0 < t1);

    c.SetPaused(true);

    plThreadUtils::Sleep(plTime::Milliseconds(10));
    c.Update();

    const plTime t2 = c.GetAccumulatedTime();
    PLASMA_TEST_BOOL(t1 == t2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetFixedTimeStep / GetFixedTimeStep")
  {
    plClock c("Test");

    PLASMA_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 0.0, 0.0);

    c.SetFixedTimeStep(plTime::Seconds(1.0 / 60.0));

    PLASMA_TEST_DOUBLE(c.GetFixedTimeStep().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Updates with fixed time step")
  {
    plClock c("Test");
    c.SetFixedTimeStep(plTime::Seconds(1.0 / 60.0));
    c.Update();

    plThreadUtils::Sleep(plTime::Milliseconds(10));

    c.Update();
    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    plThreadUtils::Sleep(plTime::Milliseconds(50));

    c.Update();
    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);

    c.Update();
    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 1.0 / 60.0, 0.000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetAccumulatedTime / GetAccumulatedTime")
  {
    plClock c("Test");

    c.SetAccumulatedTime(plTime::Seconds(23.42));

    PLASMA_TEST_DOUBLE(c.GetAccumulatedTime().GetSeconds(), 23.42, 0.000001);

    c.Update(); // by default after a SetAccumulatedTime the time diff should always be > 0

    PLASMA_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);

    const plTime t0 = c.GetAccumulatedTime();

    plThreadUtils::Sleep(plTime::Milliseconds(5));
    c.Update();

    const plTime t1 = c.GetAccumulatedTime();

    PLASMA_TEST_BOOL(t1 > t0);
    PLASMA_TEST_BOOL(c.GetTimeDiff().GetSeconds() > 0.0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetSpeed / GetSpeed / GetTimeDiff")
  {
    plClock c("Test");
    PLASMA_TEST_DOUBLE(c.GetSpeed(), 1.0, 0.0);

    c.SetFixedTimeStep(plTime::Seconds(0.01));

    c.SetSpeed(10.0);
    PLASMA_TEST_DOUBLE(c.GetSpeed(), 10.0, 0.000001);

    c.Update();
    const plTime t0 = c.GetTimeDiff();
    PLASMA_TEST_DOUBLE(t0.GetSeconds(), 0.1, 0.00001);

    c.SetSpeed(0.1);

    c.Update();
    const plTime t1 = c.GetTimeDiff();
    PLASMA_TEST_DOUBLE(t1.GetSeconds(), 0.001, 0.00001);

    c.Reset(false);

    c.Update();
    const plTime t2 = c.GetTimeDiff();
    PLASMA_TEST_DOUBLE(t2.GetSeconds(), 0.01, 0.00001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetMinimumTimeStep / GetMinimumTimeStep")
  {
    plClock c("Test");
    PLASMA_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.001, 0.0); // to ensure the tests fail if somebody changes these constants

    c.Update();
    c.Update();

    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMinimumTimeStep().GetSeconds(), 0.0000000001);

    c.SetMinimumTimeStep(plTime::Seconds(0.1));
    c.SetMaximumTimeStep(plTime::Seconds(1.0));

    PLASMA_TEST_DOUBLE(c.GetMinimumTimeStep().GetSeconds(), 0.1, 0.0);

    c.Update();
    c.Update();

    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMinimumTimeStep().GetSeconds(), 0.0000000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetMaximumTimeStep / GetMaximumTimeStep")
  {
    plClock c("Test");
    PLASMA_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.1, 0.0); // to ensure the tests fail if somebody changes these constants

    plThreadUtils::Sleep(plTime::Milliseconds(200));
    c.Update();

    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);

    c.SetMaximumTimeStep(plTime::Seconds(0.2));

    PLASMA_TEST_DOUBLE(c.GetMaximumTimeStep().GetSeconds(), 0.2, 0.0);

    plThreadUtils::Sleep(plTime::Milliseconds(400));
    c.Update();

    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), c.GetMaximumTimeStep().GetSeconds(), 0.0000000001);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetTimeStepSmoothing / GetTimeStepSmoothing")
  {
    plClock c("Test");

    PLASMA_TEST_BOOL(c.GetTimeStepSmoothing() == nullptr);

    plSimpleTimeStepSmoother s;
    c.SetTimeStepSmoothing(&s);

    PLASMA_TEST_BOOL(c.GetTimeStepSmoothing() == &s);

    c.SetMaximumTimeStep(plTime::Seconds(10.0)); // this would limit the time step even after smoothing
    c.Update();

    PLASMA_TEST_DOUBLE(c.GetTimeDiff().GetSeconds(), 0.42, 0.0);
  }
}
