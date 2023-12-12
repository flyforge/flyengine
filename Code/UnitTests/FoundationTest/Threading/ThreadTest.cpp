#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Time/Time.h>

namespace
{
  volatile plInt32 g_iCrossThreadVariable = 0;
  const plUInt32 g_uiIncrementSteps = 160000;

  class TestThread3 : public plThread
  {
  public:
    TestThread3()
      : plThread("Test Thread")
    {
    }

    plMutex* m_pWaitMutex = nullptr;
    plMutex* m_pBlockedMutex = nullptr;

    virtual plUInt32 Run()
    {
      // test TryLock on a locked mutex
      PLASMA_TEST_BOOL(m_pBlockedMutex->TryLock().Failed());

      {
        // enter and leave the mutex once
        PLASMA_LOCK(*m_pWaitMutex);
      }

      PLASMA_PROFILE_SCOPE("Test Thread::Run");

      for (plUInt32 i = 0; i < g_uiIncrementSteps; i++)
      {
        plAtomicUtils::Increment(g_iCrossThreadVariable);

        plTime::Now();
        plThreadUtils::YieldTimeSlice();
        plTime::Now();
      }

      return 0;
    }
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST_GROUP(Threading);

PLASMA_CREATE_SIMPLE_TEST(Threading, Thread)
{
  g_iCrossThreadVariable = 0;


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Thread")
  {
    TestThread3* pTestThread31 = nullptr;
    TestThread3* pTestThread32 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
    try
    {
      pTestThread31 = new TestThread3;
      pTestThread32 = new TestThread3;
    }
    catch (...)
    {
    }

    PLASMA_TEST_BOOL(pTestThread31 != nullptr);
    PLASMA_TEST_BOOL(pTestThread32 != nullptr);

    plMutex waitMutex, blockedMutex;
    pTestThread31->m_pWaitMutex = &waitMutex;
    pTestThread32->m_pWaitMutex = &waitMutex;

    pTestThread31->m_pBlockedMutex = &blockedMutex;
    pTestThread32->m_pBlockedMutex = &blockedMutex;

    // no one holds these mutexes yet, must succeed
    PLASMA_TEST_BOOL(blockedMutex.TryLock().Succeeded());
    PLASMA_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Both thread will increment the global variable via atomic operations
    pTestThread31->Start();
    pTestThread32->Start();

    // give the threads a bit of time to start
    plThreadUtils::Sleep(plTime::Milliseconds(50));

    // allow the threads to run now
    waitMutex.Unlock();

    // Main thread will also increment the test variable
    plAtomicUtils::Increment(g_iCrossThreadVariable);

    // Join with both threads
    pTestThread31->Join();
    pTestThread32->Join();

    // we are holding the mutex, another TryLock should work
    PLASMA_TEST_BOOL(blockedMutex.TryLock().Succeeded());

    // The threads should have finished, no one holds the lock
    PLASMA_TEST_BOOL(waitMutex.TryLock().Succeeded());

    // Test deletion
    delete pTestThread31;
    delete pTestThread32;

    PLASMA_TEST_INT(g_iCrossThreadVariable, g_uiIncrementSteps * 2 + 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Thread Sleeping")
  {
    const plTime start = plTime::Now();

    plTime sleepTime(plTime::Seconds(0.3));

    plThreadUtils::Sleep(sleepTime);

    const plTime stop = plTime::Now();

    const plTime duration = stop - start;

    // We test for 0.25 - 0.35 since the threading functions are a bit varying in their precision
    PLASMA_TEST_BOOL(duration.GetSeconds() > 0.25);
    PLASMA_TEST_BOOL_MSG(duration.GetSeconds() < 1.0, "This test can fail when the machine is under too much load and blocks the process for too long.");
  }
}
