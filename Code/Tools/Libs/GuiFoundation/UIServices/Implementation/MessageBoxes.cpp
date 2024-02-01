#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void plQtUiServices::MessageBoxStatus(const plStatus& s, const char* szFailureMsg, const char* szSuccessMsg, bool bOnlySuccessMsgIfDetails)
{
  plStringBuilder sResult;

  if (s.m_Result.Succeeded())
  {
    if (plStringUtils::IsNullOrEmpty(szSuccessMsg))
      return;

    if (bOnlySuccessMsgIfDetails && s.m_sMessage.IsEmpty())
      return;

    sResult = szSuccessMsg;

    if (!s.m_sMessage.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.m_sMessage);

    MessageBoxInformation(sResult);
  }
  else
  {
    sResult = szFailureMsg;

    if (!s.m_sMessage.IsEmpty())
      sResult.AppendFormat("\n\nDetails:\n{0}", s.m_sMessage);

    MessageBoxWarning(sResult);
  }
}

void plQtUiServices::MessageBoxInformation(const plFormatString& msg)
{
  plStringBuilder tmp;

  if (s_bHeadless)
    plLog::Info(msg.GetText(tmp));
  else
  {
    QMessageBox::information(QApplication::activeWindow(), plApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), QMessageBox::StandardButton::Ok);
  }
}

void plQtUiServices::MessageBoxWarning(const plFormatString& msg)
{
  plStringBuilder tmp;

  if (s_bHeadless)
    plLog::Warning(msg.GetText(tmp));
  else
  {
    QMessageBox::warning(QApplication::activeWindow(), plApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), QMessageBox::StandardButton::Ok);
  }
}

QMessageBox::StandardButton plQtUiServices::MessageBoxQuestion(
  const plFormatString& msg, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
  if (s_bHeadless)
  {
    return defaultButton;
  }
  else
  {
    plStringBuilder tmp;

    return QMessageBox::question(QApplication::activeWindow(), plApplication::GetApplicationInstance()->GetApplicationName().GetData(), QString::fromUtf8(msg.GetTextCStr(tmp)), buttons, defaultButton);
  }
}
