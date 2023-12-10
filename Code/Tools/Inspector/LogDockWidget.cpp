#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qlistwidget.h>

plQtLogDockWidget* plQtLogDockWidget::s_pWidget = nullptr;

plQtLogDockWidget::plQtLogDockWidget(QWidget* pParent)
  : ads::CDockWidget("Log", pParent)
{
  s_pWidget = this;
  setupUi(this);
  LogWidget->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Log"));

  setIcon(QIcon(":/Icons/Icons/Log.svg"));

  this->setWidget(LogWidget);
}

void plQtLogDockWidget::ResetStats()
{
  LogWidget->GetLog()->Clear();
}

void plQtLogDockWidget::Log(const plFormatString& text)
{
  plStringBuilder tmp;

  plLogEntry lm;
  lm.m_sMsg = text.GetText(tmp);
  lm.m_Type = plLogMsgType::InfoMsg;
  lm.m_uiIndentation = 0;
  LogWidget->GetLog()->AddLogMsg(lm);
}

void plQtLogDockWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage Msg;

  while (plTelemetry::RetrieveMessage(' LOG', Msg) == PLASMA_SUCCESS)
  {
    plLogEntry lm;
    plInt8 iEventType = 0;

    Msg.GetReader() >> iEventType;
    Msg.GetReader() >> lm.m_uiIndentation;
    Msg.GetReader() >> lm.m_sTag;
    Msg.GetReader() >> lm.m_sMsg;

    if (iEventType == plLogMsgType::EndGroup)
      Msg.GetReader() >> lm.m_fSeconds;

    lm.m_Type = (plLogMsgType::Enum)iEventType;
    s_pWidget->LogWidget->GetLog()->AddLogMsg(lm);
  }
}
