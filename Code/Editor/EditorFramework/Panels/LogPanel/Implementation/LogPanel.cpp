#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>


PLASMA_IMPLEMENT_SINGLETON(plQtLogPanel);

plQtLogPanel::plQtLogPanel()
  : plQtApplicationPanel("Panel.Log")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setIcon(plQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Log.svg"));
  setWindowTitle(QString::fromUtf8(plTranslate("Panel.Log")));
  setWidget(pDummy);

  EditorLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Editor Log"));
  EngineLog->GetSearchWidget()->setPlaceholderText(QStringLiteral("Search Engine Log"));

  plGlobalLog::AddLogWriter(plMakeDelegate(&plQtLogPanel::LogWriter, this));
  PlasmaEditorEngineProcessConnection::s_Events.AddEventHandler(plMakeDelegate(&plQtLogPanel::EngineProcessMsgHandler, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    splitter->restoreState(Settings.value("Splitter", splitter->saveState()).toByteArray());
  }
  Settings.endGroup();

  connect(EditorLog->GetLog(), &plQtLogModel::NewErrorsOrWarnings, this, &plQtLogPanel::OnNewWarningsOrErrors);
  connect(EngineLog->GetLog(), &plQtLogModel::NewErrorsOrWarnings, this, &plQtLogPanel::OnNewWarningsOrErrors);

  plQtUiServices::GetSingleton()->s_Events.AddEventHandler(plMakeDelegate(&plQtLogPanel::UiServiceEventHandler, this));
}

plQtLogPanel::~plQtLogPanel()
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String("LogPanel"));
  {
    Settings.setValue("Splitter", splitter->saveState());
  }
  Settings.endGroup();

  plGlobalLog::RemoveLogWriter(plMakeDelegate(&plQtLogPanel::LogWriter, this));
  PlasmaEditorEngineProcessConnection::s_Events.RemoveEventHandler(plMakeDelegate(&plQtLogPanel::EngineProcessMsgHandler, this));
  plQtUiServices::GetSingleton()->s_Events.RemoveEventHandler(plMakeDelegate(&plQtLogPanel::UiServiceEventHandler, this));
}

void plQtLogPanel::OnNewWarningsOrErrors(const char* szText, bool bError)
{
  m_uiKnownNumWarnings = EditorLog->GetLog()->GetNumSeriousWarnings() + EditorLog->GetLog()->GetNumWarnings() +
                         EngineLog->GetLog()->GetNumSeriousWarnings() + EngineLog->GetLog()->GetNumWarnings();
  m_uiKnownNumErrors = EditorLog->GetLog()->GetNumErrors() + EngineLog->GetLog()->GetNumErrors();

  plQtUiServices::Event::TextType type = plQtUiServices::Event::Info;

  plUInt32 uiShowNumWarnings = 0;
  plUInt32 uiShowNumErrors = 0;

  if (m_uiKnownNumWarnings > m_uiIgnoreNumWarnings)
  {
    uiShowNumWarnings = m_uiKnownNumWarnings - m_uiIgnoreNumWarnings;
    type = plQtUiServices::Event::Warning;
  }
  else
  {
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;
  }

  if (m_uiKnownNumErrors > m_uiIgnoredNumErrors)
  {
    uiShowNumErrors = m_uiKnownNumErrors - m_uiIgnoredNumErrors;
    type = plQtUiServices::Event::Error;
  }
  else
  {
    m_uiIgnoredNumErrors = m_uiKnownNumErrors;
  }

  plStringBuilder tmp;
  if (uiShowNumErrors > 0)
  {
    tmp.AppendFormat("{} Errors", uiShowNumErrors);
  }
  if (uiShowNumWarnings > 0)
  {
    tmp.AppendWithSeparator(", ", "");
    tmp.AppendFormat("{} Warnings", uiShowNumWarnings);
  }

  plQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(tmp, type);

  if (!plStringUtils::IsNullOrEmpty(szText))
  {
    plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
      plFmt("{}: {}", bError ? "Error" : "Warning", szText), plTime::Seconds(10));
  }
}

void plQtLogPanel::ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectClosing:
      EditorLog->GetLog()->Clear();
      EngineLog->GetLog()->Clear();
      [[fallthrough]];

    case plToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == plToolsProjectEvent::Type::ProjectOpened);
      break;

    default:
      break;
  }

  plQtApplicationPanel::ToolsProjectEventHandler(e);
}

void plQtLogPanel::LogWriter(const plLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  plLogEntry msg(e);
  EditorLog->GetLog()->AddLogMsg(msg);

  if (msg.m_sTag == "EditorStatus")
  {
    plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt(msg.m_sMsg), plTime::Seconds(5));
  }
}

void plQtLogPanel::EngineProcessMsgHandler(const PlasmaEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case PlasmaEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (const plLogMsgToEditor* pMsg = plDynamicCast<const plLogMsgToEditor*>(e.m_pMsg))
      {
        EngineLog->GetLog()->AddLogMsg(pMsg->m_Entry);
      }
    }
    break;

    default:
      return;
  }
}

void plQtLogPanel::UiServiceEventHandler(const plQtUiServices::Event& e)
{
  if (e.m_Type == plQtUiServices::Event::ClickedDocumentPermanentStatusBarText)
  {
    EnsureVisible();

    m_uiIgnoredNumErrors = m_uiKnownNumErrors;
    m_uiIgnoreNumWarnings = m_uiKnownNumWarnings;

    plQtUiServices::GetSingleton()->ShowAllDocumentsPermanentStatusBarMessage(nullptr, plQtUiServices::Event::Info);
  }
}
