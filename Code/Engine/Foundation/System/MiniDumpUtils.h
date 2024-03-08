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

  /// \brief Specifies the dump mode that is written.
  enum class plDumpType
  {
    Auto, ///< Uses the setting specified globally through the command line.
    MiniDump, ///< Saves a mini-dump without full memory, regardless of this application's command line flag '-fullcrashdumps'.
    MiniDumpWithFullMemory ///< Saves a mini-dump with full memory, regardless of this application's command line flag '-fullcrashdumps'.
  };

  /// \brief Tries to write a mini-dump for the external process with the given process ID.
  ///
  /// \sa WriteProcessMiniDump()
  static plStatus WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plDumpType dumpTypeOverride = plDumpType::Auto);

  /// \brief Tries to launch pl's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  ///
  /// \note On Windows: The command line option '-fullcrashdumps' is passed if either set in this application's command line or if overridden through dumpTypeOverride = plDumpType::MiniDumpWithFullMemory.
  static plStatus LaunchMiniDumpTool(plStringView sDumpFile, plDumpType dumpTypeOverride = plDumpType::Auto);

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  static plStatus WriteOwnProcessMiniDump(plStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo, plDumpType dumpTypeOverride = plDumpType::Auto);

  /// \brief Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  static plMinWindows::HANDLE GetProcessHandleWithNecessaryRights(plUInt32 uiProcessID);

  /// \brief Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  static plStatus WriteExternalProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plMinWindows::HANDLE hProcess, plDumpType dumpTypeOverride = plDumpType::Auto);

  /// \brief Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: A crash-dump with a full memory capture is made if either this application's command line option '-fullcrashdumps' is specified or if that setting is overridden through dumpTypeOverride = plDumpType::MiniDumpWithFullMemory.
  static plStatus WriteProcessMiniDump(plStringView sDumpFile, plUInt32 uiProcessID, plMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo, plDumpType dumpTypeOverrideType = plDumpType::Auto);

#endif
};