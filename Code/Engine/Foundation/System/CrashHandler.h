#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>

/// \brief Helper class to manage the top level exception handler.
///
/// A default exception handler is provided but not set by default.
/// The default implementation will write the exception and callstack to the output
/// and create a memory dump using WriteDump that create a dump file in the folder
/// specified via SetExceptionHandler.

/// \brief This class allows to hook into the OS top-level exception handler to handle application crashes
///
/// Derive from this class to implement custom behavior. Call plCrashHandler::SetCrashHandler() to
/// register which instance to use.
///
/// For typical use-cases use plCrashHandler_WriteMiniDump::g_Instance.
class PL_FOUNDATION_DLL plCrashHandler
{
public:
  plCrashHandler();
  virtual ~plCrashHandler();

  static void SetCrashHandler(plCrashHandler* pHandler);
  static plCrashHandler* GetCrashHandler();

  virtual void HandleCrash(void* pOsSpecificData) = 0;

private:
  static plCrashHandler* s_pActiveHandler;
};

/// \brief A default implementation of plCrashHandler that tries to write a mini-dump and prints the callstack.
///
/// To use it, call plCrashHandler::SetCrashHandler(&plCrashHandler_WriteMiniDump::g_Instance);
/// Do not forget to also specify the dump-file path, otherwise writing dump-files is skipped.
class PL_FOUNDATION_DLL plCrashHandler_WriteMiniDump : public plCrashHandler
{
public:
  static plCrashHandler_WriteMiniDump g_Instance;

  struct PathFlags
  {
    using StorageType = plUInt8;

    enum Enum
    {
      AppendDate = PL_BIT(0),      ///< Whether to append the current date to the crash-dump file (YYYY-MM-DD_HH-MM-SS)
      AppendSubFolder = PL_BIT(1), ///< Whether to append "CrashDump" as a sub-folder
      AppendPID = PL_BIT(2),       ///< Whether to append the process ID to the crash-dump file

      Default = AppendDate | AppendSubFolder | AppendPID
    };

    struct Bits
    {
      StorageType AppendDate : 1;
      StorageType AppendSubFolder : 1;
      StorageType AppendPID : 1;
    };
  };

public:
  plCrashHandler_WriteMiniDump();

  /// \brief Sets the raw path for the dump-file to write
  void SetFullDumpFilePath(plStringView sFullAbsDumpFilePath);

  /// \brief Sets the dump-file path to "{szAbsDirectoryPath}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(plStringView sAbsDirectoryPath, plStringView sAppName, plBitflags<PathFlags> flags = PathFlags::Default);

  /// \brief Sets the dump-file path to "{plOSFile::GetApplicationDirectory()}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(plStringView sAppName, plBitflags<PathFlags> flags = PathFlags::Default);

  virtual void HandleCrash(void* pOsSpecificData) override;

protected:
  virtual bool WriteOwnProcessMiniDump(void* pOsSpecificData);
  virtual void PrintStackTrace(void* pOsSpecificData);

  plString m_sDumpFilePath;
};

PL_DECLARE_FLAGS_OPERATORS(plCrashHandler_WriteMiniDump::PathFlags);
