#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ConditionVariable.h>

#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  class TestThread : public plThread
  {
  public:
    TestThread()
      : plThread("Test Thread")
    {
    }

    plConditionVariable* m_pCV = nullptr;
    plAtomicInteger32* m_pCounter = nullptr;

    virtual plUInt32 Run()
    {
      PLASMA_LOCK(*m_pCV);

      m_pCounter->Decrement();

      m_pCV->UnlockWaitForSignalAndLock();

      m_pCounter->Increment();
      return 0;
    }
  };

  class TestThreadTimeout : public plThread
  {
  public:
    TestThreadTimeout()
      : plThread("Test Thread Timeout")
    {
    }

    plConditionVariable* m_pCV = nullptr;
    plConditionVariable* m_pCVTimeout = nullptr;
    plAtomicInteger32* m_pCounter = nullptr;

    virtual plUInt32 Run()
    {
      // make sure all threads are put to sleep first
      {
        PLASMA_LOCK(*m_pCV);
        m_pCounter->Decrement();
        m_pCV->UnlockWaitForSignalAndLock();
      }

      // this condition will never be met during the test
      // it should always run into the timeout
      PLASMA_LOCK(*m_pCVTimeout);
      m_pCVTimeout->UnlockWaitForSignalAndLock(plTime::Seconds(0.5));

      m_pCounter->Increment();
      return 0;
    }
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Threading, ConditionalVariable)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Wait No Timeout")
  {
    constexpr plUInt32 uiNumThreads = 32;

    plUniquePtr<TestThread> pTestThreads[uiNumThreads];
    plAtomicInteger32 iCounter = uiNumThreads;
    plConditionVariable cv;

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i] = PLASMA_DEFAULT_NEW(TestThread);
      pTestThreads[i]->m_pCounter = &iCounter;
      pTestThreads[i]->m_pCV = &cv;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      PLASMA_LOCK(cv);
      if (iCounter == 0)
        break;

      plThreadUtils::YieldTimeSlice();
    }

    for (plUInt32 t = 0; t < uiNumThreads / 2; ++t)
    {
      const plInt32 iExpected = t + 1;

      cv.SignalOne();

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
      cv.SignalAll();

      for (plUInt32 a = 0; a < 1000; ++a)
      {
        plThreadUtils::Sleep(plTime::Milliseconds(1));

        if (iCounter >= (plInt32)uiNumThreads)
          break;
      }

      // theoretically this could fail, if the OS doesn't wake up any other thread in time
      // but with 1000 tries that is very unlikely
      PLASMA_TEST_INT(iCounter, (plInt32)uiNumThreads);
      PLASMA_TEST_BOOL(iCounter <= (plInt32)uiNumThreads); // THIS test must never fail!
    }

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Wait With timeout")
  {
    constexpr plUInt32 uiNumThreads = 16;

    plUniquePtr<TestThreadTimeout> pTestThreads[uiNumThreads];
    plAtomicInteger32 iCounter = uiNumThreads;
    plConditionVariable cv;
    plConditionVariable cvt;

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i] = PLASMA_DEFAULT_NEW(TestThreadTimeout);
      pTestThreads[i]->m_pCounter = &iCounter;
      pTestThreads[i]->m_pCV = &cv;
      pTestThreads[i]->m_pCVTimeout = &cvt;
      pTestThreads[i]->Start();
    }

    // wait until all threads are in waiting state
    while (true)
    {
      // We need to lock here as otherwise we could signal
      // while a thread hasn't reached the wait yet.
      PLASMA_LOCK(cv);
      if (iCounter == 0)
        break;

      plThreadUtils::YieldTimeSlice();
    }

    // open the flood gates
    cv.SignalAll();

    // all threads should run into their timeout now
    for (plUInt32 a = 0; a < 100; ++a)
    {
      plThreadUtils::Sleep(plTime::Milliseconds(50));

      if (iCounter >= (plInt32)uiNumThreads)
        break;
    }

    // theoretically this could fail, if the OS doesn't wake up any other thread in time
    // but with 100 tries that is very unlikely
    PLASMA_TEST_INT(iCounter, (plInt32)uiNumThreads);
    PLASMA_TEST_BOOL(iCounter <= (plInt32)uiNumThreads); // THIS test must never fail!

    for (plUInt32 i = 0; i < uiNumThreads; ++i)
    {
      pTestThreads[i]->Join();
    }
  }
}
