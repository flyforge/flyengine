#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>

static plInt32 iTestData1 = 0;
static plInt32 iTestData2 = 0;

// The following event handlers are automatically registered, nothing else needs to be done here

PLASMA_ON_GLOBAL_EVENT(TestGlobalEvent1)
{
  iTestData1 += param0.Get<plInt32>();
}

PLASMA_ON_GLOBAL_EVENT(TestGlobalEvent2)
{
  iTestData2 += param0.Get<plInt32>();
}

PLASMA_ON_GLOBAL_EVENT_ONCE(TestGlobalEvent3)
{
  // this handler will be executed only once, even if the event is broadcast multiple times
  iTestData2 += 42;
}

static bool g_bFirstRun = true;

PLASMA_CREATE_SIMPLE_TEST(Communication, GlobalEvent)
{
  iTestData1 = 0;
  iTestData2 = 0;

  PLASMA_TEST_INT(iTestData1, 0);
  PLASMA_TEST_INT(iTestData2, 0);

  plGlobalEvent::Broadcast("TestGlobalEvent1", 1);

  PLASMA_TEST_INT(iTestData1, 1);
  PLASMA_TEST_INT(iTestData2, 0);

  plGlobalEvent::Broadcast("TestGlobalEvent1", 2);

  PLASMA_TEST_INT(iTestData1, 3);
  PLASMA_TEST_INT(iTestData2, 0);

  plGlobalEvent::Broadcast("TestGlobalEvent1", 3);

  PLASMA_TEST_INT(iTestData1, 6);
  PLASMA_TEST_INT(iTestData2, 0);

  plGlobalEvent::Broadcast("TestGlobalEvent2", 4);

  PLASMA_TEST_INT(iTestData1, 6);
  PLASMA_TEST_INT(iTestData2, 4);

  plGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  PLASMA_TEST_INT(iTestData1, 6);

  if (g_bFirstRun)
  {
    g_bFirstRun = false;
    PLASMA_TEST_INT(iTestData2, 46);
  }
  else
  {
    PLASMA_TEST_INT(iTestData2, 4);
    iTestData2 += 42;
  }

  plGlobalEvent::Broadcast("TestGlobalEvent2", 5);

  PLASMA_TEST_INT(iTestData1, 6);
  PLASMA_TEST_INT(iTestData2, 51);

  plGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  PLASMA_TEST_INT(iTestData1, 6);
  PLASMA_TEST_INT(iTestData2, 51);

  plGlobalEvent::Broadcast("TestGlobalEvent2", 6);

  PLASMA_TEST_INT(iTestData1, 6);
  PLASMA_TEST_INT(iTestData2, 57);

  plGlobalEvent::Broadcast("TestGlobalEvent3", 4);

  PLASMA_TEST_INT(iTestData1, 6);
  PLASMA_TEST_INT(iTestData2, 57);

  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);

  plGlobalEvent::PrintGlobalEventStatistics();

  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
}
