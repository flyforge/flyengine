#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread2 : public plThread
  {
  public:
    TestThread2()
      : plThread("Test Thread")
    {
    }

    plThreadSignal* m_pSignalAuto = nullptr;
    plThreadSignal* m_pSignalManual = nullptr;
    plAtomicInteger32* m_pCounter = nullptr;
    bool m_bTimeout = false;

    virtual plUInt32 Run()
    {
      m_pCounter->Decrement();

      m_pSignalAuto->WaitForSignal();

      m_pCounter->Increment();

      if (m_bTimeout)
      {
        m_pSignalManual->WaitForSignal(plTime::Seconds(0.5));
      }
      else
      {
        m_pSignalManual->WaitForSignal();
      }

      m_pCounter->Increment();

      return 0;
    }
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Threading, ThreadSignal)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr plUInt32 uiNumThreads = 32;

    plUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    plAtomicInteger32 iCounter = uiNumThreads;
    plThreadSignal sigAuto(plThreadSignal::Mode::AutoReset);
    plThreadSignal sigManual(plThreadSignal::Mode::ManualReset);

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i] = PLASMA_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter = &iCounter;
      pTestThread2s[i]->m_pSignalAuto = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      plThreadUtils::YieldTimeSlice();
    }

    for (plUInt32 t = 0; t < uiNumThreads; ++t)
    {
      const plInt32 iExpected = t + 1;

      sigAuto.RaiseSignal();

      for (plUInt32 a = 0; a < 1000; ++a)
      {
        plThreadUtils::Sleep(plTime::Milliseconds(1));

        if (iCounter >= iExpected)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      PLASMA_TEST_INT(iCounter, iExpected);
      PLASMA_TEST_BOOL(iCounter <= iExpected); // THIS test must never fail!
    }

    // wake up the rest
    {
      sigManual.RaiseSignal();

      for (plUInt32 a = 0; a < 1000; ++a)
      {
        plThreadUtils::Sleep(plTime::Milliseconds(1));

        if (iCounter >= (plInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      PLASMA_TEST_INT(iCounter, (plInt32)uiNumThreads * 2);
      PLASMA_TEST_BOOL(iCounter <= (plInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Wait With Timeout")
  {
    constexpr plUInt32 uiNumThreads = 16;

    plUniquePtr<TestThread2> pTestThread2s[uiNumThreads];
    plAtomicInteger32 iCounter = uiNumThreads;
    plThreadSignal sigAuto(plThreadSignal::Mode::AutoReset);
    plThreadSignal sigManual(plThreadSignal::Mode::ManualReset);

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i] = PLASMA_DEFAULT_NEW(TestThread2);
      pTestThread2s[i]->m_pCounter = &iCounter;
      pTestThread2s[i]->m_pSignalAuto = &sigAuto;
      pTestThread2s[i]->m_pSignalManual = &sigManual;
      pTestThread2s[i]->m_bTimeout = true;
      pTestThread2s[i]->Start();
    }

    // wait until all threads are in waiting state
    while (iCounter > 0)
    {
      plThreadUtils::YieldTimeSlice();
    }

    // raise the signal N times
    for (plUInt32 t = 0; t < uiNumThreads; ++t)
    {
      sigAuto.RaiseSignal();

      for (plUInt32 a = 0; a < 1000; ++a)
      {
        plThreadUtils::Sleep(plTime::Milliseconds(1));

        if (iCounter >= (plInt32)t + 1)
          break;
      }
    }

    // due to the wait timeout in the thread, testing this exact value here would be unreliable
    // PLASMA_TEST_INT(iCounter, (plInt32)uiNumThreads);

    // just wait for the rest
    {
      for (plUInt32 a = 0; a < 100; ++a)
      {
        plThreadUtils::Sleep(plTime::Milliseconds(50));

        if (iCounter >= (plInt32)uiNumThreads * 2)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      PLASMA_TEST_INT(iCounter, (plInt32)uiNumThreads * 2);
      PLASMA_TEST_BOOL(iCounter <= (plInt32)uiNumThreads * 2); // THIS test must never fail!
    }

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThread2s[i]->Join();
    }
  }
}
