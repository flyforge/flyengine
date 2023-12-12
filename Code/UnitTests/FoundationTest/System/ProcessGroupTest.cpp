#include <FoundationTest/FoundationTestPCH.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_PROCESSES)

#  include <Foundation/System/ProcessGroup.h>
#  include <Foundation/Utilities/CommandLineUtils.h>

PLASMA_CREATE_SIMPLE_TEST(System, ProcessGroup)
{
  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const plStringBuilder pathToSelf = plCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "WaitToFinish")
  {
    plProcessGroup pgroup;
    plStringBuilder out;

    plMutex mutex;

    for (plUInt32 i = 0; i < 8; ++i)
    {
      plProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_onStdOut = [&out, &mutex](plStringView sView) {
        PLASMA_LOCK(mutex);
        out.Append(sView);
      };

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("1000");
      opt.m_Arguments.PushBack("-stdout");
      opt.m_Arguments.PushBack("Na");

      PLASMA_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    // in a debugger with child debugging enabled etc. even 10 seconds can lead to timeouts due to long delays in the IDE
    PLASMA_TEST_BOOL(pgroup.WaitToFinish(plTime::Seconds(60)).Succeeded());
    PLASMA_TEST_STRING(out, "NaNaNaNaNaNaNaNa"); // BATMAN!
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "TerminateAll")
  {
    plProcessGroup pgroup;

    plHybridArray<plProcess, 8> procs;

    for (plUInt32 i = 0; i < 8; ++i)
    {
      plProcessOptions opt;
      opt.m_sProcess = pathToSelf;

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("60000");

      PLASMA_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    const plTime tStart = plTime::Now();
    PLASMA_TEST_BOOL(pgroup.TerminateAll().Succeeded());
    const plTime tDiff = plTime::Now() - tStart;

    PLASMA_TEST_BOOL(tDiff < plTime::Seconds(10));
  }
}
#endif
