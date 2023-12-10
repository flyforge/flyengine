#pragma once
#include <Foundation/Logging/Log.h>
#include <TestFramework/TestFrameworkDLL.h>

/// \brief An plLogInterface that expects and handles error messages during test runs. Can be
/// used to ensure that expected error messages are produced by the tested functionality.
/// Expected error messages are not passed on and do not cause tests to fail.
class PLASMA_TEST_DLL plTestLogInterface : public plLogInterface
{
public:
  plTestLogInterface() = default;
  ~plTestLogInterface();
  virtual void HandleLogMessage(const plLoggingEventData& le) override;

  /// \brief Add expected message. Will fail the test when the expected message is not
  /// encountered. Can take an optional count, if messages are expected multiple times
  void ExpectMessage(const char* szMsg, plLogMsgType::Enum type = plLogMsgType::All, plInt32 iCount = 1);

  /// \brief Set the log interface that unhandled messages are forwarded to.
  void SetParentLog(plLogInterface* pInterface) { m_pParentLog = pInterface; }

private:
  plLogInterface* m_pParentLog = nullptr;

  struct ExpectedMsg
  {
    plInt32 m_iCount = 0;
    plString m_sMsgSubString;
    plLogMsgType::Enum m_Type = plLogMsgType::All;
  };

  mutable plMutex m_Mutex;
  plHybridArray<ExpectedMsg, 8> m_ExpectedMessages;
};

/// \brief A class that sets a custom plTestLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope. The test version passes the previous
/// plLogInterface on to the plTestLogInterface to enable passing on unhandled messages.
///
/// If bCatchMessagesGlobally is false, the system only intercepts messages on the current thread.
/// If bCatchMessagesGlobally is true, it will also intercept messages from other threads, as long as they
/// go through plGlobalLog. See plGlobalLog::SetGlobalLogOverride().
class PLASMA_TEST_DLL plTestLogSystemScope : public plLogSystemScope
{
public:
  explicit plTestLogSystemScope(plTestLogInterface* pInterface, bool bCatchMessagesGlobally = false)
    : plLogSystemScope(pInterface)
  {
    m_bCatchMessagesGlobally = bCatchMessagesGlobally;
    pInterface->SetParentLog(m_pPrevious);

    if (m_bCatchMessagesGlobally)
    {
      plGlobalLog::SetGlobalLogOverride(pInterface);
    }
  }

  ~plTestLogSystemScope()
  {
    if (m_bCatchMessagesGlobally)
    {
      plGlobalLog::SetGlobalLogOverride(nullptr);
    }
  }

private:
  bool m_bCatchMessagesGlobally = false;
};
