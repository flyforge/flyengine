#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Console/Console.h>
#include <EditorFramework/Panels/CVarPanel/CVarPanel.moc.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

PL_IMPLEMENT_SINGLETON(plQtCVarPanel);

class plCommandInterpreterFwd : public plCommandInterpreter
{
public:
  virtual void Interpret(plCommandInterpreterState& inout_state) override
  {
    plConsoleCmdMsgToEngine msg;
    msg.m_iType = 0;
    msg.m_sCommand = inout_state.m_sInput;

    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  virtual void AutoComplete(plCommandInterpreterState& inout_state) override
  {
    plConsoleCmdMsgToEngine msg;
    msg.m_iType = 1;
    msg.m_sCommand = inout_state.m_sInput;

    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
};

plQtCVarPanel::plQtCVarPanel()
  : plQtApplicationPanel("Panel.CVar")
  , m_SingletonRegistrar(this)
{
  setIcon(plQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/CVar.svg"));
  setWindowTitle(plMakeQString(plTranslate("Panel.CVar")));
  m_pCVarWidget = new plQtCVarWidget(this);
  m_pCVarWidget->layout()->setContentsMargins(0, 0, 0, 0);
  // m_pCVarWidget->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pCVarWidget);

  plEditorEngineProcessConnection::s_Events.AddEventHandler(plMakeDelegate(&plQtCVarPanel::EngineProcessMsgHandler, this));

  connect(m_pCVarWidget, &plQtCVarWidget::onBoolChanged, this, &plQtCVarPanel::BoolChanged);
  connect(m_pCVarWidget, &plQtCVarWidget::onFloatChanged, this, &plQtCVarPanel::FloatChanged);
  connect(m_pCVarWidget, &plQtCVarWidget::onIntChanged, this, &plQtCVarPanel::IntChanged);
  connect(m_pCVarWidget, &plQtCVarWidget::onStringChanged, this, &plQtCVarPanel::StringChanged);

  m_pCVarWidget->GetConsole().SetCommandInterpreter(PL_DEFAULT_NEW(plCommandInterpreterFwd));
}

plQtCVarPanel::~plQtCVarPanel()
{
  plEditorEngineProcessConnection::s_Events.RemoveEventHandler(plMakeDelegate(&plQtCVarPanel::EngineProcessMsgHandler, this));
}

void plQtCVarPanel::ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case plToolsProjectEvent::Type::ProjectClosing:
      m_EngineCVarState.Clear();
      m_pCVarWidget->Clear();

      [[fallthrough]];

    case plToolsProjectEvent::Type::ProjectOpened:
      setEnabled(e.m_Type == plToolsProjectEvent::Type::ProjectOpened);
      break;

    default:
      break;
  }

  plQtApplicationPanel::ToolsProjectEventHandler(e);
}

void plQtCVarPanel::EngineProcessMsgHandler(const plEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case plEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<plCVarMsgToEditor>())
      {
        const plCVarMsgToEditor* pMsg = static_cast<const plCVarMsgToEditor*>(e.m_pMsg);

        bool bExisted = false;
        auto& cvar = m_EngineCVarState.FindOrAdd(pMsg->m_sName, &bExisted).Value();
        cvar.m_sDescription = pMsg->m_sDescription;
        cvar.m_sPlugin = pMsg->m_sPlugin;

        switch (pMsg->m_Value.GetType())
        {
          case plVariantType::Float:
            cvar.m_uiType = plCVarType::Float;
            cvar.m_fValue = pMsg->m_Value.ConvertTo<float>();
            break;
          case plVariantType::Int32:
            cvar.m_uiType = plCVarType::Int;
            cvar.m_iValue = pMsg->m_Value.ConvertTo<int>();
            break;
          case plVariantType::Bool:
            cvar.m_uiType = plCVarType::Bool;
            cvar.m_bValue = pMsg->m_Value.ConvertTo<bool>();
            break;
          case plVariantType::String:
            cvar.m_uiType = plCVarType::String;
            cvar.m_sValue = pMsg->m_Value.ConvertTo<plString>();
            break;
          default:
            break;
        }

        if (!bExisted)
          m_bRebuildUI = true;

        if (!m_bUpdateUI)
        {
          m_bUpdateUI = true;

          // don't do this every single time, otherwise we would spam this during project load
          QTimer::singleShot(100, this, SLOT(UpdateUI()));
        }
      }
      else if (auto pMsg = plDynamicCast<const plConsoleCmdResultMsgToEditor*>(e.m_pMsg))
      {
        m_sCommandResult.Append(pMsg->m_sResult.GetView());
        m_bUpdateConsole = true;
        QTimer::singleShot(100, this, SLOT(UpdateUI()));
      }
    }
    break;
    default:
      break;
  }
}

void plQtCVarPanel::UpdateUI()
{
  if (m_bRebuildUI)
  {
    m_pCVarWidget->RebuildCVarUI(m_EngineCVarState);
  }
  else if (m_bUpdateUI)
  {
    m_pCVarWidget->UpdateCVarUI(m_EngineCVarState);
  }

  if (m_bUpdateConsole)
  {
    m_pCVarWidget->AddConsoleStrings(m_sCommandResult);
  }

  m_sCommandResult.Clear();
  m_bUpdateConsole = false;
  m_bUpdateUI = false;
  m_bRebuildUI = false;
}

void plQtCVarPanel::BoolChanged(const char* szCVar, bool newValue)
{
  plChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtCVarPanel::FloatChanged(const char* szCVar, float newValue)
{
  plChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtCVarPanel::IntChanged(const char* szCVar, int newValue)
{
  plChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtCVarPanel::StringChanged(const char* szCVar, const char* newValue)
{
  plChangeCVarMsgToEngine msg;
  msg.m_sCVarName = szCVar;
  msg.m_NewValue = newValue;
  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
