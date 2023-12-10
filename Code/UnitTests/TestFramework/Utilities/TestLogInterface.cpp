#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestLogInterface.h>

#include <TestFramework/Framework/TestFramework.h>

plTestLogInterface::~plTestLogInterface()
{
  for (const ExpectedMsg& msg : m_ExpectedMessages)
  {
    plInt32 count = msg.m_iCount;
    PLASMA_TEST_BOOL_MSG(count == 0, "Message \"%s\" was logged %d times %s than expected.", msg.m_sMsgSubString.GetData(), count < 0 ? -count : count,
      count < 0 ? "more" : "less");
  }
}

void plTestLogInterface::HandleLogMessage(const plLoggingEventData& le)
{
  {
    // in case this interface is used with plTestLogSystemScope to override the plGlobalLog (see plGlobalLog::SetGlobalLogOverride)
    // it must be thread-safe
    PLASMA_LOCK(m_Mutex);

    for (ExpectedMsg& msg : m_ExpectedMessages)
    {
      if (msg.m_Type != plLogMsgType::All && le.m_EventType != msg.m_Type)
        continue;

      if (le.m_sText.FindSubString(msg.m_sMsgSubString))
      {
        --msg.m_iCount;

        // filter out error and warning messages entirely
        if (le.m_EventType >= plLogMsgType::ErrorMsg && le.m_EventType <= plLogMsgType::WarningMsg)
          return;

        // pass all other messages along to the parent log
        break;
      }
    }
  }

  if (m_pParentLog)
  {
    m_pParentLog->HandleLogMessage(le);
  }
}

void plTestLogInterface::ExpectMessage(const char* szMsg, plLogMsgType::Enum type /*= plLogMsgType::All*/, plInt32 iCount /*= 1*/)
{
  PLASMA_LOCK(m_Mutex);

  // Do not allow initial count to be less than 1, but use signed int to keep track
  // of error messages that were encountered more often than expected.
  PLASMA_ASSERT_DEV(iCount >= 1, "Message needs to be expected at least once");

  ExpectedMsg& em = m_ExpectedMessages.ExpandAndGetRef();
  em.m_sMsgSubString = szMsg;
  em.m_iCount = iCount;
  em.m_Type = type;
}


PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Utilities_TestLogInterface);
