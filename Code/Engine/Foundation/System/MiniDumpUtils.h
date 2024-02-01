#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Types/Status.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
extern "C"
{
  struct _EXCEPTION_POINTERS;
}
#endif

/// \brief Functionality for writing process mini-dumps (callstacks, memory snapshots, etc)
struct PL_FOUNDATION_DLL plMiniDumpUtils
{
  /// \brief Tries to write a mini-dump for the external process with the given process ID.
  ///
  /// \sa WriteProcessMiniDump()
  static plStatus WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID);

  /// \brief Tries to launch pl's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  ///
  /// \note On Windows: If the command line option '-fullcrashdumps' is specified, it is forwarded to the MiniDumpTool.
  static plStatus LaunchMiniDumpTool(plStringView sDumpFile);

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  static plStatus WriteOwnProcessMiniDump(plStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo);

  /// \brief Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  static plMinWindows::HANDLE GetProcessHandleWithNecessaryRights(plUInt32 uiProcessID);

  /// \brief Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  static plStatus WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plMinWindows::HANDLE hProcess);

  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: If the command line option '-fullcrashdumps' is specified, a crash-dump with a full memory capture is made.
  static plStatus WriteProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo);

#endif
};
