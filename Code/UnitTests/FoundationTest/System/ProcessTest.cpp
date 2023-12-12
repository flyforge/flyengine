#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(System);

#if PLASMA_ENABLED(PLASMA_SUPPORTS_PROCESSES)

PLASMA_CREATE_SIMPLE_TEST(System, Process)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Command Line")
  {
    plProcessOptions proc;
    proc.m_Arguments.PushBack("-bla");
    proc.m_Arguments.PushBack("blub blub");
    proc.m_Arguments.PushBack("\"di dub\"");
    proc.AddArgument(" -test ");
    proc.AddArgument("-hmpf {}", 27);
    proc.AddCommandLine("-a b   -c  d  -e \"f g h\" ");

    plStringBuilder cmdLine;
    proc.BuildCommandLineString(cmdLine);

    PLASMA_TEST_STRING(cmdLine, "-bla \"blub blub\" \"di dub\" -test \"-hmpf 27\" -a b -c d -e \"f g h\"");
  }

  static const char* g_szTestMsg = "Tell me more!\nAnother line\n520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\nThat's all";
  static const char* g_szTestMsgLine0 = "Tell me more!\n";
  static const char* g_szTestMsgLine1 = "Another line\n";
  static const char* g_szTestMsgLine2 = "520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\n";
  static const char* g_szTestMsgLine3 = "That's all";


  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const plStringBuilder pathToSelf = plCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Execute")
  {
    plProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    plInt32 exitCode = -1;

    if (!PLASMA_TEST_BOOL_MSG(plProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
      return;

    PLASMA_TEST_INT(exitCode, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Launch / WaitToFinish")
  {
    plProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    plProcess proc;
    PLASMA_TEST_BOOL(proc.GetState() == plProcessState::NotStarted);

    if (!PLASMA_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
      return;

    PLASMA_TEST_BOOL(proc.GetState() == plProcessState::Running);
    PLASMA_TEST_BOOL(proc.WaitToFinish(plTime::Seconds(5)).Succeeded());
    PLASMA_TEST_BOOL(proc.GetState() == plProcessState::Finished);
    PLASMA_TEST_INT(proc.GetExitCode(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Launch / Terminate")
  {
    plProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("10000");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("0");

    plProcess proc;
    PLASMA_TEST_BOOL(proc.GetState() == plProcessState::NotStarted);

    if (!PLASMA_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
      return;

    PLASMA_TEST_BOOL(proc.GetState() == plProcessState::Running);
    PLASMA_TEST_BOOL(proc.Terminate().Succeeded());
    PLASMA_TEST_BOOL(proc.GetState() == plProcessState::Finished);
    PLASMA_TEST_INT(proc.GetExitCode(), -1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Launch / Detach")
  {
    plTime tTerminate;

    {
      plProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("10000");

      plProcess proc;
      if (!PLASMA_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
        return;

      proc.Detach();

      tTerminate = plTime::Now();
    }

    const plTime tDiff = plTime::Now() - tTerminate;
    PLASMA_TEST_BOOL_MSG(tDiff < plTime::Seconds(1.0), "Destruction of plProcess should be instant after Detach() was used.");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STDOUT")
  {
    plDynamicArray<plStringBuilder> lines;
    plStringBuilder out;
    plProcessOptions opt;
    opt.m_onStdOut = [&](plStringView sView) {
      out.Append(sView);
      lines.PushBack(sView);
    };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!PLASMA_TEST_BOOL_MSG(plProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (PLASMA_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    PLASMA_TEST_STRING(out, g_szTestMsg);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STDERROR")
  {
    plStringBuilder err;
    plProcessOptions opt;
    opt.m_onStdError = [&err](plStringView sView) { err.Append(sView); };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stderr");
    opt.m_Arguments.PushBack("NOT A VALID COMMAND");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("1");

    plInt32 exitCode = 0;

    if (!PLASMA_TEST_BOOL_MSG(plProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
      return;

    PLASMA_TEST_BOOL_MSG(!err.IsEmpty(), "Error stream should contain something.");
    PLASMA_TEST_INT(exitCode, 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STDOUT_STDERROR")
  {
    plDynamicArray<plStringBuilder> lines;
    plStringBuilder out;
    plStringBuilder err;
    plProcessOptions opt;
    opt.m_onStdOut = [&](plStringView sView) {
      out.Append(sView);
      lines.PushBack(sView);
    };
    opt.m_onStdError = [&err](plStringView sView) { err.Append(sView); };
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!PLASMA_TEST_BOOL_MSG(plProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (PLASMA_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      PLASMA_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    PLASMA_TEST_STRING(out, g_szTestMsg);
    PLASMA_TEST_BOOL_MSG(err.IsEmpty(), "Error stream should be empty.");
  }
}
#endif
