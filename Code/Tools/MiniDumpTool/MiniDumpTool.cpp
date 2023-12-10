#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

plCommandLineOptionInt opt_PID("_MiniDumpTool", "-PID", "Process ID of the application for which to create a crash dump.", 0);

plCommandLineOptionPath opt_DumpFile("_MiniDumpTool", "-f", "Path to the crash dump file to write.", "");

class plMiniDumpTool : public plApplication
{
  plUInt32 m_uiProcessID = 0;
  plStringBuilder m_sDumpFile;

public:
  typedef plApplication SUPER;

  plMiniDumpTool()
    : plApplication("MiniDumpTool")
  {
  }

  plResult ParseArguments()
  {
    plCommandLineUtils* cmd = plCommandLineUtils::GetGlobalInstance();

    m_uiProcessID = cmd->GetUIntOption("-PID");

    m_sDumpFile = opt_DumpFile.GetOptionValue(plCommandLineOption::LogMode::Always);
    m_sDumpFile.MakeCleanPath();

    if (m_uiProcessID == 0)
    {
      plLog::Error("Missing '-PID' argument");
      return PLASMA_FAILURE;
    }

    return PLASMA_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add the empty data directory to access files via absolute paths
    plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

    plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual Execution Run() override
  {
    {
      plStringBuilder cmdHelp;
      if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested, "_MiniDumpTool"))
      {
        plLog::Print(cmdHelp);
        return plApplication::Execution::Quit;
      }
    }

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return plApplication::Execution::Quit;
    }

    plMiniDumpUtils::WriteExternalProcessMiniDump(m_sDumpFile, m_uiProcessID).IgnoreResult();
    return plApplication::Execution::Quit;
  }
};

PLASMA_CONSOLEAPP_ENTRY_POINT(plMiniDumpTool);
