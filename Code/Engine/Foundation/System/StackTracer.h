#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Types/Delegate.h>

/// \brief Helper class to capture the current stack and print a captured stack
class PL_FOUNDATION_DLL plStackTracer
{
public:
  /// \brief Captures the current stack trace.
  ///
  /// The trace will contain not more than ref_trace.GetCount() entries.
  /// [Windows] If called in an exception handler, set pContext to PEXCEPTION_POINTERS::ContextRecord.
  /// Returns the actual number of captured entries.
  static plUInt32 GetStackTrace(plArrayPtr<void*>& ref_trace, void* pContext = nullptr);

  /// \brief Callback-function to print a text somewhere
  using PrintFunc = plDelegate<void(const char* szText)>;

  /// \brief Print a stack trace
  static void ResolveStackTrace(const plArrayPtr<void*>& trace, PrintFunc printFunc);

  /// \brief Print a stack trace without resolving it
  static void PrintStackTrace(const plArrayPtr<void*>& trace, PrintFunc printFunc);

private:
  plStackTracer() = delete;

  static void OnPluginEvent(const plPluginEvent& e);

  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, StackTracer);
};
