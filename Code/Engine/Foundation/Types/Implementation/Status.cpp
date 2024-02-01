#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Status.h>

void plResult::AssertSuccess(const char* szMsg /*= nullptr*/, const char* szDetails /*= nullptr*/) const
{
  if (Succeeded())
    return;

  if (szMsg)
  {
    PL_REPORT_FAILURE(szMsg, szDetails);
  }
  else
  {
    PL_REPORT_FAILURE("An operation failed unexpectedly.");
  }
}

plStatus::plStatus(const plFormatString& fmt)
  : m_Result(PL_FAILURE)
{
  plStringBuilder sMsg;
  m_sMessage = fmt.GetText(sMsg);
}

bool plStatus::LogFailure(plLogInterface* pLog)
{
  if (Failed())
  {
    plLogInterface* pInterface = pLog ? pLog : plLog::GetThreadLocalLogSystem();
    plLog::Error(pInterface, "{0}", m_sMessage);
  }

  return Failed();
}

void plStatus::AssertSuccess(const char* szMsg /*= nullptr*/) const
{
  if (Succeeded())
    return;

  if (szMsg)
  {
    PL_REPORT_FAILURE(szMsg, m_sMessage.GetData());
  }
  else
  {
    PL_REPORT_FAILURE("An operation failed unexpectedly.", m_sMessage.GetData());
  }
}


