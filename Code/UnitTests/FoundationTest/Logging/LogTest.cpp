#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Threading/Thread.h>
#include <TestFramework/Utilities/TestLogInterface.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Logging);

namespace
{

  class LogTestLogInterface : public plLogInterface
  {
  public:
    virtual void HandleLogMessage(const plLoggingEventData& le) override
    {
      switch (le.m_EventType)
      {
        case plLogMsgType::Flush:
          m_Result.Append("[Flush]\n");
          return;
        case plLogMsgType::BeginGroup:
          m_Result.Append(">", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::EndGroup:
          m_Result.Append("<", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::ErrorMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::SeriousWarningMsg:
          m_Result.Append("SW:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::WarningMsg:
          m_Result.Append("W:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::SuccessMsg:
          m_Result.Append("S:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::InfoMsg:
          m_Result.Append("I:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::DevMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::DebugMsg:
          m_Result.Append("D:", le.m_sTag, " ", le.m_sText, "\n");
          break;

        default:
          PLASMA_REPORT_FAILURE("Invalid msg type");
          break;
      }
    }

    plStringBuilder m_Result;
  };

} // namespace

PLASMA_CREATE_SIMPLE_TEST(Logging, Log)
{
  LogTestLogInterface log;
  LogTestLogInterface log2;
  plLogSystemScope logScope(&log);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Output")
  {
    PLASMA_LOG_BLOCK("Verse 1", "Portal: Still Alive");

    plLog::GetThreadLocalLogSystem()->SetLogLevel(plLogMsgType::All);

    plLog::Success("{0}", "This was a triumph.");
    plLog::Info("{0}", "I'm making a note here:");
    plLog::Error("{0}", "Huge Success");
    plLog::Info("{0}", "It's hard to overstate my satisfaction.");
    plLog::Dev("{0}", "Aperture Science. We do what we must, because we can,");
    plLog::Dev("{0}", "For the good of all of us, except the ones who are dead.");
    plLog::Flush();
    plLog::Flush(); // second flush should be ignored

    {
      PLASMA_LOG_BLOCK("Verse 2");

      plLog::GetThreadLocalLogSystem()->SetLogLevel(plLogMsgType::DevMsg);

      plLog::Dev("But there's no sense crying over every mistake.");
      plLog::Debug("You just keep on trying 'till you run out of cake.");
      plLog::Info("And the science gets done, and you make a neat gun");
      plLog::Error("for the people who are still alive.");
    }

    {
      PLASMA_LOG_BLOCK("Verse 3");

      plLog::GetThreadLocalLogSystem()->SetLogLevel(plLogMsgType::InfoMsg);

      plLog::Info("I'm not even angry.");
      plLog::Debug("I'm being so sincere right now.");
      plLog::Dev("Even though you broke my heart and killed me.");
      plLog::Info("And tore me to pieces,");
      plLog::Dev("and threw every piece into a fire.");
      plLog::Info("As they burned it hurt because I was so happy for you.");
      plLog::Error("Now these points of data make a beautiful line");
      plLog::Dev("and we're off the beta, we're releasing on time.");
      plLog::Flush();
      plLog::Flush();

      {
        PLASMA_LOG_BLOCK("Verse 4");

        plLog::GetThreadLocalLogSystem()->SetLogLevel(plLogMsgType::SuccessMsg);

        plLog::Info("So I'm glad I got burned,");
        plLog::Debug("think of all the things we learned");
        plLog::Debug("for the people who are still alive.");

        {
          plLogSystemScope logScope2(&log2);
          PLASMA_LOG_BLOCK("Interlude");
          plLog::Info("Well here we are again. It's always such a pleasure.");
          plLog::Error("Remember when you tried to kill me twice?");
        }

        {
          PLASMA_LOG_BLOCK("Verse 5");

          plLog::GetThreadLocalLogSystem()->SetLogLevel(plLogMsgType::WarningMsg);

          plLog::Debug("Go ahead and leave me.");
          plLog::Info("I think I prefer to stay inside.");
          plLog::Dev("Maybe you'll find someone else, to help you.");
          plLog::Dev("Maybe Black Mesa.");
          plLog::Info("That was a joke. Haha. Fat chance.");
          plLog::Warning("Anyway, this cake is great.");
          plLog::Success("It's so delicious and moist.");
          plLog::Dev("Look at me still talking when there's science to do.");
          plLog::Error("When I look up there it makes me glad I'm not you.");
          plLog::Info("I've experiments to run,");
          plLog::SeriousWarning("there is research to be done on the people who are still alive.");
        }
      }
    }
  }

  {
    PLASMA_LOG_BLOCK("Verse 6", "Last One");

    plLog::GetThreadLocalLogSystem()->SetLogLevel(plLogMsgType::ErrorMsg);

    plLog::Dev("And believe me I am still alive.");
    plLog::Info("I'm doing science and I'm still alive.");
    plLog::Success("I feel fantastic and I'm still alive.");
    plLog::Warning("While you're dying I'll be still alive.");
    plLog::Error("And when you're dead I will be, still alive.");
    plLog::Debug("Still alive, still alive.");
  }

  /// \todo This test will fail if PLASMA_COMPILE_FOR_DEVELOPMENT is disabled.
  /// We also currently don't test plLog::Debug, because our build machines compile in release and then the text below would need to be
  /// different.

  const char* szResult = log.m_Result;
  const char* szExpected = "\
>Portal: Still Alive Verse 1\n\
S: This was a triumph.\n\
I: I'm making a note here:\n\
E: Huge Success\n\
I: It's hard to overstate my satisfaction.\n\
E: Aperture Science. We do what we must, because we can,\n\
E: For the good of all of us, except the ones who are dead.\n\
[Flush]\n\
> Verse 2\n\
E: But there's no sense crying over every mistake.\n\
I: And the science gets done, and you make a neat gun\n\
E: for the people who are still alive.\n\
< Verse 2\n\
> Verse 3\n\
I: I'm not even angry.\n\
I: And tore me to pieces,\n\
I: As they burned it hurt because I was so happy for you.\n\
E: Now these points of data make a beautiful line\n\
[Flush]\n\
> Verse 4\n\
> Verse 5\n\
W: Anyway, this cake is great.\n\
E: When I look up there it makes me glad I'm not you.\n\
SW: there is research to be done on the people who are still alive.\n\
< Verse 5\n\
< Verse 4\n\
< Verse 3\n\
<Portal: Still Alive Verse 1\n\
>Last One Verse 6\n\
E: And when you're dead I will be, still alive.\n\
<Last One Verse 6\n\
";

  PLASMA_TEST_STRING(szResult, szExpected);

  const char* szResult2 = log2.m_Result;
  const char* szExpected2 = "\
> Interlude\n\
I: Well here we are again. It's always such a pleasure.\n\
E: Remember when you tried to kill me twice?\n\
< Interlude\n\
";

  PLASMA_TEST_STRING(szResult2, szExpected2);
}

PLASMA_CREATE_SIMPLE_TEST(Logging, GlobalTestLog)
{
  plLog::GetThreadLocalLogSystem()->SetLogLevel(plLogMsgType::All);

  {
    plTestLogInterface log;
    plTestLogSystemScope scope(&log, true);

    log.ExpectMessage("managed to break", plLogMsgType::ErrorMsg);
    log.ExpectMessage("my heart", plLogMsgType::WarningMsg);
    log.ExpectMessage("see you", plLogMsgType::WarningMsg, 10);

    {
      class LogThread : public plThread
      {
      public:
        virtual plUInt32 Run() override
        {
          plLog::Warning("I see you!");
          plLog::Debug("Test debug");
          return 0;
        }
      };

      LogThread thread[10];

      for (plUInt32 i = 0; i < 10; ++i)
      {
        thread[i].Start();
      }

      plLog::Error("The only thing you managed to break so far");
      plLog::Warning("is my heart");

      for (plUInt32 i = 0; i < 10; ++i)
      {
        thread[i].Join();
      }
    }
  }
}
