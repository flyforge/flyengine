#include <CoreTest/CoreTestPCH.h>

#include <Core/Utils/IntervalScheduler.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Utils);

namespace
{
  struct TestWork
  {
    float m_IntervalMs = 0.0f;
    plUInt32 m_Counter = 0;

    void Run()
    {
      ++m_Counter;
    }
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Utils, IntervalScheduler)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constant workload")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    plHybridArray<TestWork, 32> works;
    plIntervalScheduler<TestWork*> scheduler;

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(intervals); ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, plTime::Milliseconds(work.m_IntervalMs));
    }

    constexpr plUInt32 uiNumIterations = 60;

    plUInt32 wrongDelta = 0;
    for (plUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(plTime::Milliseconds(10), [&](TestWork* pWork, plTime deltaTime) {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = pWork->m_IntervalMs * 0.3;
          const double midValue = pWork->m_IntervalMs + 1.0 - variance;
          if (plMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      PLASMA_TEST_FLOAT(fNumWorks, 2.5f, 0.5f);

      for (auto& work : works)
      {
        PLASMA_TEST_BOOL(scheduler.GetInterval(&work) == plTime::Milliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~120 scheduled works is ok
    PLASMA_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = 600.0f / plMath::Max(work.m_IntervalMs, 10.0f);

      // check for roughly expected or a little bit more
      PLASMA_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 3.0f, 4.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constant workload (bigger delta)")
  {
    float intervals[] = {10, 20, 60, 60, 60};

    plHybridArray<TestWork, 32> works;
    plIntervalScheduler<TestWork*> scheduler;

    for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(intervals); ++i)
    {
      auto& work = works.ExpandAndGetRef();
      work.m_IntervalMs = intervals[i];

      scheduler.AddOrUpdateWork(&work, plTime::Milliseconds(work.m_IntervalMs));
    }

    constexpr plUInt32 uiNumIterations = 60;

    plUInt32 wrongDelta = 0;
    for (plUInt32 i = 0; i < uiNumIterations; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(plTime::Milliseconds(20), [&](TestWork* pWork, plTime deltaTime) {
        if (i > 10)
        {
          const double deltaMs = deltaTime.GetMilliseconds();
          const double variance = plMath::Max(pWork->m_IntervalMs, 20.0f) * 0.3;
          const double midValue = plMath::Max(pWork->m_IntervalMs, 20.0f) + 1.0 - variance;
          if (plMath::IsEqual<double>(deltaMs, midValue, variance) == false)
          {
            ++wrongDelta;
          }
        }

        pWork->Run();
        ++fNumWorks; });

      PLASMA_TEST_FLOAT(fNumWorks, 3.5f, 0.5f);

      for (auto& work : works)
      {
        PLASMA_TEST_BOOL(scheduler.GetInterval(&work) == plTime::Milliseconds(work.m_IntervalMs));
      }
    }

    // 3 wrong deltas for ~150 scheduled works is ok
    PLASMA_TEST_BOOL(wrongDelta <= 3);

    for (auto& work : works)
    {
      const float expectedCounter = 1200.0f / plMath::Max(work.m_IntervalMs, 20.0f);

      // check for roughly expected or a little bit more
      PLASMA_TEST_FLOAT(static_cast<float>(work.m_Counter), expectedCounter + 2.0f, 3.0f);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Dynamic workload")
  {
    plHybridArray<TestWork, 32> works;

    plIntervalScheduler<TestWork*> scheduler;

    for (plUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, plTime::Milliseconds(i));
    }

    for (plUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0;
      scheduler.Update(plTime::Milliseconds(10), [&](TestWork* pWork, plTime deltaTime) {
        pWork->Run();
        ++fNumWorks; });

      PLASMA_TEST_FLOAT(fNumWorks, 15.5f, 0.5f);
    }

    for (plUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works.ExpandAndGetRef();
      scheduler.AddOrUpdateWork(&work, plTime::Milliseconds(20 + i));
    }

    float fPrevNumWorks = 15.5f;
    for (plUInt32 i = 0; i < 60; ++i)
    {
      float fNumWorks = 0.0f;
      scheduler.Update(plTime::Milliseconds(10), [&](TestWork* pWork, plTime deltaTime) {
        pWork->Run();
        ++fNumWorks; });

      // fNumWork will slowly ramp up until it reaches the new workload of 22 or 23 per update
      PLASMA_TEST_BOOL(fNumWorks + 1.0f >= fPrevNumWorks);
      PLASMA_TEST_BOOL(fNumWorks <= 23.0f);

      fPrevNumWorks = fNumWorks;
    }

    for (plUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i];
      scheduler.RemoveWork(&work);
    }

    scheduler.Update(plTime::Milliseconds(10), plIntervalScheduler<TestWork*>::RunWorkCallback());

    for (plUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      PLASMA_TEST_BOOL(scheduler.GetInterval(&work) == plTime::Milliseconds(20 + i));

      scheduler.AddOrUpdateWork(&work, plTime::Milliseconds(100 + i));
    }

    scheduler.Update(plTime::Milliseconds(10), plIntervalScheduler<TestWork*>::RunWorkCallback());

    for (plUInt32 i = 0; i < 16; ++i)
    {
      auto& work = works[i + 16];
      PLASMA_TEST_BOOL(scheduler.GetInterval(&work) == plTime::Milliseconds(100 + i));
    }
  }
}
