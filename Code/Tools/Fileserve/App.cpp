#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Utilities/CommandLineUtils.h>

void plFileserverApp::AfterCoreSystemsStartup()
{
  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  // Add the empty data directory to access files via absolute paths
  plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

  PLASMA_DEFAULT_NEW(plFileserver);

  plFileserver::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plFileserverApp::FileserverEventHandler, this));

#ifndef PLASMA_USE_QT
  plFileserver::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plFileserverApp::FileserverEventHandlerConsole, this));
  plFileserver::GetSingleton()->StartServer();
#endif

  // TODO: CommandLine Option
  m_CloseAppTimeout = plTime::Seconds(plCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_close_timeout", 0));
  m_TimeTillClosing = plTime::Seconds(plCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_wait_timeout", 0));

  if (m_TimeTillClosing.GetSeconds() > 0)
  {
    m_TimeTillClosing += plTime::Now();
  }
}

void plFileserverApp::BeforeCoreSystemsShutdown()
{
  plFileserver::GetSingleton()->StopServer();

#ifndef PLASMA_USE_QT
  plFileserver::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plFileserverApp::FileserverEventHandlerConsole, this));
#endif

  plFileserver::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plFileserverApp::FileserverEventHandler, this));

  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

plApplication::Execution plFileserverApp::Run()
{
  // if there are no more connections, and we have a timeout to close when no connections are left, we return Quit
  if (m_uiConnections == 0 && m_TimeTillClosing > plTime::Seconds(0) && plTime::Now() > m_TimeTillClosing)
  {
    return plApplication::Execution::Quit;
  }

  if (plFileserver::GetSingleton()->UpdateServer() == false)
  {
    m_uiSleepCounter++;

    if (m_uiSleepCounter > 1000)
    {
      // only sleep when no work had to be done in a while
      plThreadUtils::Sleep(plTime::Milliseconds(10));
    }
    else if (m_uiSleepCounter > 10)
    {
      // only sleep when no work had to be done in a while
      plThreadUtils::Sleep(plTime::Milliseconds(1));
    }
  }
  else
  {
    m_uiSleepCounter = 0;
  }

  return plApplication::Execution::Continue;
}
