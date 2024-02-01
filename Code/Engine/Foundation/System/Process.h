#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

using plOsProcessHandle = void*;
using plOsProcessID = plUInt32;

#if PL_ENABLED(PL_SUPPORTS_PROCESSES)
enum class plProcessState
{
  NotStarted,
  Running,
  Finished
};

/// \brief Options that describe how to run an external process
struct PL_FOUNDATION_DLL plProcessOptions
{
  /// Path to the binary to launch
  plString m_sProcess;

  /// Custom working directory for the launched process. If empty, inherits the CWD from the parent process.
  plString m_sWorkingDirectory;

  /// Arguments to pass to the process. Strings that contain spaces will be wrapped in quotation marks automatically
  plHybridArray<plString, 8> m_Arguments;

  /// If set to true, command line tools will not show their console window, but execute in the background
  bool m_bHideConsoleWindow = true;

  /// If set, stdout will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  plDelegate<void(plStringView)> m_onStdOut;

  /// If set, stderr will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  plDelegate<void(plStringView)> m_onStdError;

  /// \brief Appends a formatted argument to m_Arguments
  ///
  /// This can be useful if a complex command needs to be added as a single argument.
  /// Ie. since arguments with spaces will be wrapped in quotes, it can make a difference
  /// whether a complex parameter is added as one or multiple arguments.
  void AddArgument(const plFormatString& arg);

  /// \brief Overload of AddArgument(plFormatString) for convenience.
  template <typename... ARGS>
  void AddArgument(plStringView sFormat, ARGS&&... args)
  {
    AddArgument(plFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Takes a full command line and appends it as individual arguments by splitting it along white-space and quotation marks.
  ///
  /// Brief, use this, if arguments are already pre-built as a full command line.
  void AddCommandLine(plStringView sCmdLine);

  /// \brief Builds the command line from the process arguments and appends it to \a out_sCmdLine.
  void BuildCommandLineString(plStringBuilder& out_sCmdLine) const;
};

/// \brief Flags for plProcess::Launch()
struct plProcessLaunchFlags
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = 0,
    Detached = PL_BIT(0),  ///< The process will be detached right after launch, as if plProcess::Detach() was called.
    Suspended = PL_BIT(1), ///< The process will be launched in a suspended state. Call plProcess::ResumeSuspended() to unpause it.
    Default = None
  };

  struct Bits
  {
    StorageType Detached : 1;
    StorageType Suspended : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plProcessLaunchFlags);

/// \brief Provides functionality to launch other processes
class PL_FOUNDATION_DLL plProcess
{
  PL_DISALLOW_COPY_AND_ASSIGN(plProcess);

public:
  plProcess();
  plProcess(plProcess&& rhs);

  /// \brief Upon destruction the running process will be terminated.
  ///
  /// Use Detach() to prevent the termination of the launched process.
  ///
  /// \sa Terminate()
  /// \sa Detach()
  ~plProcess();

  /// \brief Launches the specified process and waits for it to finish.
  static plResult Execute(const plProcessOptions& opt, plInt32* out_pExitCode = nullptr);

  /// \brief Launches the specified process asynchronously.
  ///
  /// When the function returns, the process is typically starting or running.
  /// Call WaitToFinish() to wait for the process to shutdown or Terminate() to kill it.
  ///
  /// \sa plProcessLaunchFlags
  plResult Launch(const plProcessOptions& opt, plBitflags<plProcessLaunchFlags> launchFlags = plProcessLaunchFlags::None);

  /// \brief Resumes a process that was launched in a suspended state. Returns PL_FAILURE if the process has not been launched or already
  /// resumed.
  plResult ResumeSuspended();

  /// \brief Waits the given amount of time for the previously launched process to finish.
  ///
  /// Pass in plTime::MakeZero() to wait indefinitely.
  /// Returns PL_FAILURE, if the process did not finish within the given time.
  ///
  /// \note Asserts that the plProcess instance was used to successfully launch a process before.
  plResult WaitToFinish(plTime timeout = plTime::MakeZero());

  /// \brief Kills the detached process, if possible.
  plResult Terminate();

  /// \brief Returns the exit code of the process. The exit code will be -0xFFFF as long as the process has not finished.
  plInt32 GetExitCode() const;

  /// \brief Returns the running state of the process
  ///
  /// If the state is 'finished' the exit code (as returned by GetExitCode() ) will be updated.
  plProcessState GetState() const;

  /// \brief Detaches the running process from the plProcess instance.
  ///
  /// This means the plProcess instance loses control over terminating the process or communicating with it.
  /// It also means that the process will keep running and not get terminated when the plProcess instance is destroyed.
  void Detach();

  /// \brief Returns the OS specific handle to the process
  plOsProcessHandle GetProcessHandle() const;

  /// \brief Returns the OS-specific process ID (PID)
  plOsProcessID GetProcessID() const;

  /// \brief Returns OS-specific process ID (PID) for the calling process
  static plOsProcessID GetCurrentProcessID();

private:
  void BuildFullCommandLineString(const plProcessOptions& opt, plStringView sProcess, plStringBuilder& cmd) const;

  plUniquePtr<struct plProcessImpl> m_pImpl;

  // the default value is used by GetExitCode() to determine whether it has to be reevaluated
  mutable plInt32 m_iExitCode = -0xFFFF;

  plString m_sProcess;
  plDelegate<void(plStringView)> m_OnStdOut;
  plDelegate<void(plStringView)> m_OnStdError;
  mutable plTime m_ProcessExited = plTime::MakeZero();
};
#endif
