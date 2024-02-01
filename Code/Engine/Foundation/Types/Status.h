#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class plLogInterface;

/// \brief An plResult with an additional message for the reason of failure
struct [[nodiscard]] PL_FOUNDATION_DLL plStatus
{
  PL_ALWAYS_INLINE explicit plStatus()
    : m_Result(PL_FAILURE)
  {
  }

  // const char* version is needed for disambiguation
  explicit plStatus(const char* szError)
    : m_Result(PL_FAILURE)
    , m_sMessage(szError)
  {
  }

  explicit plStatus(plResult r, plStringView sError)
    : m_Result(r)
    , m_sMessage(sError)
  {
  }

  explicit plStatus(plStringView sError)
    : m_Result(PL_FAILURE)
    , m_sMessage(sError)
  {
  }

  PL_ALWAYS_INLINE plStatus(plResult r)
    : m_Result(r)
  {
  }

  explicit plStatus(const plFormatString& fmt);

  [[nodiscard]] PL_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  [[nodiscard]] PL_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  PL_ALWAYS_INLINE void IgnoreResult()
  {
    /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// \brief If the state is PL_FAILURE, the message is written to the given log (or the currently active thread-local log).
  ///
  /// The return value is the same as 'Failed()' but isn't marked as [[nodiscard]], ie returns true, if a failure happened.
  bool LogFailure(plLogInterface* pLog = nullptr);

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a msg is given, this will be the assert message.
  /// Additionally m_sMessage will be included as a detailed message.
  void AssertSuccess(const char* szMsg = nullptr) const;

  plResult m_Result;
  plString m_sMessage;
};

PL_ALWAYS_INLINE plResult plToResult(const plStatus& result)
{
  return result.m_Result;
}
