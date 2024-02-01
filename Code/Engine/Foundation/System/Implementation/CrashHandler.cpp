#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Timestamp.h>

//////////////////////////////////////////////////////////////////////////

plCrashHandler* plCrashHandler::s_pActiveHandler = nullptr;

plCrashHandler::plCrashHandler() = default;

plCrashHandler::~plCrashHandler()
{
  if (s_pActiveHandler == this)
  {
    SetCrashHandler(nullptr);
  }
}

plCrashHandler* plCrashHandler::GetCrashHandler()
{
  return s_pActiveHandler;
}

//////////////////////////////////////////////////////////////////////////

plCrashHandler_WriteMiniDump plCrashHandler_WriteMiniDump::g_Instance;

plCrashHandler_WriteMiniDump::plCrashHandler_WriteMiniDump() = default;

void plCrashHandler_WriteMiniDump::SetFullDumpFilePath(plStringView sFullAbsDumpFilePath)
{
  m_sDumpFilePath = sFullAbsDumpFilePath;
}

void plCrashHandler_WriteMiniDump::SetDumpFilePath(plStringView sAbsDirectoryPath, plStringView sAppName, plBitflags<PathFlags> flags)
{
  plStringBuilder sOutputPath = sAbsDirectoryPath;

  if (flags.IsSet(PathFlags::AppendSubFolder))
  {
    sOutputPath.AppendPath("CrashDumps");
  }

  sOutputPath.AppendPath(sAppName);

  if (flags.IsSet(PathFlags::AppendDate))
  {
    const plDateTime date = plDateTime::MakeFromTimestamp(plTimestamp::CurrentTimestamp());
    sOutputPath.AppendFormat("_{}", date);
  }

#if PL_ENABLED(PL_SUPPORTS_PROCESSES)
  if (flags.IsSet(PathFlags::AppendPID))
  {
    const plUInt32 pid = plProcess::GetCurrentProcessID();
    sOutputPath.AppendFormat("_{}", pid);
  }
#endif

  sOutputPath.Append(".dmp");

  SetFullDumpFilePath(sOutputPath);
}

void plCrashHandler_WriteMiniDump::SetDumpFilePath(plStringView sAppName, plBitflags<PathFlags> flags)
{
  SetDumpFilePath(plOSFile::GetApplicationDirectory(), sAppName, flags);
}

void plCrashHandler_WriteMiniDump::HandleCrash(void* pOsSpecificData)
{
  bool crashDumpWritten = false;
  if (!m_sDumpFilePath.IsEmpty())
  {
#if PL_ENABLED(PL_SUPPORTS_CRASH_DUMPS)
    if (plMiniDumpUtils::LaunchMiniDumpTool(m_sDumpFilePath).Failed())
    {
      plLog::Print("Could not launch MiniDumpTool, trying to write crash-dump from crashed process directly.\n");

      crashDumpWritten = WriteOwnProcessMiniDump(pOsSpecificData);
    }
    else
    {
      crashDumpWritten = true;
    }
#else
    crashDumpWritten = WriteOwnProcessMiniDump(pOsSpecificData);
#endif
  }
  else
  {
    plLog::Print("plCrashHandler_WriteMiniDump: No dump-file location specified.\n");
  }

  PrintStackTrace(pOsSpecificData);

  if (crashDumpWritten)
  {
    plLog::Printf("Application crashed. Crash-dump written to '%s'\n.", m_sDumpFilePath.GetData());
  }
}


