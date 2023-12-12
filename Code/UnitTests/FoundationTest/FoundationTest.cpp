#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>
#include <iostream>

plInt32 plConstructionCounter::s_iConstructions = 0;
plInt32 plConstructionCounter::s_iDestructions = 0;
plInt32 plConstructionCounter::s_iConstructionsLast = 0;
plInt32 plConstructionCounter::s_iDestructionsLast = 0;

plInt32 plConstructionCounterRelocatable::s_iConstructions = 0;
plInt32 plConstructionCounterRelocatable::s_iDestructions = 0;
plInt32 plConstructionCounterRelocatable::s_iConstructionsLast = 0;
plInt32 plConstructionCounterRelocatable::s_iDestructionsLast = 0;

PLASMA_TESTFRAMEWORK_ENTRY_POINT_BEGIN("FoundationTest", "Foundation Tests")
{
  plCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, plCommandLineUtils::PreferOsArgs);

  // if the -cmd switch is set, FoundationTest.exe will execute a couple of simple operations and then close
  // this is used to test process launching (e.g. plProcess)
  if (cmd.GetBoolOption("-cmd"))
  {
    // print something to stdout
    plStringView sStdOut = cmd.GetStringOption("-stdout");
    if (!sStdOut.IsEmpty())
    {
      plStringBuilder tmp;
      std::cout << sStdOut.GetData(tmp);
    }

    plStringView sStdErr = cmd.GetStringOption("-stderr");
    if (!sStdErr.IsEmpty())
    {
      plStringBuilder tmp;
      std::cerr << sStdErr.GetData(tmp);
    }

    // wait a little
    plThreadUtils::Sleep(plTime::Milliseconds(cmd.GetIntOption("-sleep")));

    // shutdown with exit code
    plTestSetup::DeInitTestFramework(true);
    return cmd.GetIntOption("-exitcode");
  }
}
PLASMA_TESTFRAMEWORK_ENTRY_POINT_END()
