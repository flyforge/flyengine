#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/IO/Implementation/Win/DosDevicePath_win.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/ProcessGroup.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

#  include <Dbghelp.h>
#  include <Shlwapi.h>
#  include <tchar.h>
#  include <werapi.h>

plCommandLineOptionBool opt_FullCrashDumps("app", "-fullcrashdumps", "If enabled, crash dumps will contain the full memory image.", false);

using MINIDUMPWRITEDUMP = BOOL(WINAPI*)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

plMinWindows::HANDLE plMiniDumpUtils::GetProcessHandleWithNecessaryRights(plUInt32 uiProcessID)
{
  // try to get more than we need
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, uiProcessID);

  if (hProcess == NULL)
  {
    // try to get all that we need for a nice dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, FALSE, uiProcessID);
  }

  if (hProcess == NULL)
  {
    // try to get rights for a limited dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, uiProcessID);
  }

  return hProcess;
}

plStatus plMiniDumpUtils::WriteProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  HMODULE hDLL = ::LoadLibraryA("dbghelp.dll");

  if (hDLL == nullptr)
  {
    return plStatus("dbghelp.dll could not be loaded.");
  }

  MINIDUMPWRITEDUMP MiniDumpWriteDumpFunc = (MINIDUMPWRITEDUMP)::GetProcAddress(hDLL, "MiniDumpWriteDump");

  if (MiniDumpWriteDumpFunc == nullptr)
  {
    return plStatus("'MiniDumpWriteDump' function address could not be resolved.");
  }

  plUInt32 dumpType = MiniDumpWithHandleData | MiniDumpWithModuleHeaders | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData |
                      MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo;

  if (opt_FullCrashDumps.GetOptionValue(plCommandLineOption::LogMode::Always))
  {
    dumpType |= MiniDumpWithFullMemory;
  }

  // make sure the target folder exists
  {
    plStringBuilder folder = sDumpFile;
    folder.PathParentDirectory();
    if (plOSFile::CreateDirectoryStructure(folder).Failed())
      return plStatus("Failed to create output directory structure.");
  }

  HANDLE hFile = CreateFileW(plDosDevicePath(sDumpFile), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    return plStatus(plFmt("Creating dump file '{}' failed (Error: '{}').", sDumpFile, plArgErrorCode(GetLastError())));
  }

  PLASMA_SCOPE_EXIT(CloseHandle(hFile););

  MINIDUMP_EXCEPTION_INFORMATION exceptionParam;
  exceptionParam.ThreadId = GetCurrentThreadId(); // only valid for WriteOwnProcessMiniDump()
  exceptionParam.ExceptionPointers = pExceptionInfo;
  exceptionParam.ClientPointers = TRUE;

  if (MiniDumpWriteDumpFunc(
        hProcess, uiProcessID, hFile, (MINIDUMP_TYPE)dumpType, pExceptionInfo != nullptr ? &exceptionParam : nullptr, nullptr, nullptr) == FALSE)
  {
    return plStatus(plFmt("Writing dump file failed: '{}'.", plArgErrorCode(GetLastError())));
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plMiniDumpUtils::WriteOwnProcessMiniDump(plStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  return WriteProcessMiniDump(sDumpFile, GetCurrentProcessId(), GetCurrentProcess(), pExceptionInfo);
}

plStatus plMiniDumpUtils::WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plMinWindows::HANDLE hProcess)
{
  return WriteProcessMiniDump(sDumpFile, uiProcessID, hProcess, nullptr);
}

#endif

plStatus plMiniDumpUtils::WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  HANDLE hProcess = plMiniDumpUtils::GetProcessHandleWithNecessaryRights(uiProcessID);

  if (hProcess == nullptr)
  {
    return plStatus("Cannot access process for mini-dump writing (PID invalid or not enough rights).");
  }

  return WriteProcessMiniDump(sDumpFile, uiProcessID, hProcess, nullptr);

#else
  return plStatus("Not implemented on UPW");
#endif
}

plStatus plMiniDumpUtils::LaunchMiniDumpTool(plStringView sDumpFile)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  plStringBuilder sDumpToolPath = plOSFile::GetApplicationDirectory();
  sDumpToolPath.AppendPath("MiniDumpTool.exe");
  sDumpToolPath.MakeCleanPath();

  if (!plOSFile::ExistsFile(sDumpToolPath))
    return plStatus(plFmt("MiniDumpTool.exe not found in '{}'", sDumpToolPath));

  plProcessOptions procOpt;
  procOpt.m_sProcess = sDumpToolPath;
  procOpt.m_Arguments.PushBack("-PID");
  procOpt.AddArgument("{}", plProcess::GetCurrentProcessID());
  procOpt.m_Arguments.PushBack("-f");
  procOpt.m_Arguments.PushBack(sDumpFile);

  if (opt_FullCrashDumps.GetOptionValue(plCommandLineOption::LogMode::Always))
  {
    // forward the '-fullcrashdumps' command line argument
    procOpt.AddArgument("-fullcrashdumps");
  }

  plProcessGroup proc;
  if (proc.Launch(procOpt).Failed())
    return plStatus(plFmt("Failed to launch '{}'", sDumpToolPath));

  if (proc.WaitToFinish().Failed())
    return plStatus("Waiting for MiniDumpTool to finish failed.");

  return plStatus(PLASMA_SUCCESS);

#else
  return plStatus("Not implemented on UPW");
#endif
}
