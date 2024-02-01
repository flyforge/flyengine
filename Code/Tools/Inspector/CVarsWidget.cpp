#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qspinbox.h>

class plCommandInterpreterInspector : public plCommandInterpreter
{
public:
  virtual void Interpret(plCommandInterpreterState& inout_state) override
  {
    plTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'EXEC');
    Msg.GetWriter() << inout_state.m_sInput;
    plTelemetry::SendToServer(Msg);
  }

  virtual void AutoComplete(plCommandInterpreterState& inout_state) override
  {
    plTelemetryMessage Msg;
    Msg.SetMessageID('CMD', 'COMP');
    Msg.GetWriter() << inout_state.m_sInput;
    plTelemetry::SendToServer(Msg);
  }
};

plQtCVarsWidget* plQtCVarsWidget::s_pWidget = nullptr;

plQtCVarsWidget::plQtCVarsWidget(QWidget* pParent)
  : ads::CDockWidget("CVars", pParent)
{
  s_pWidget = this;

  setupUi(this);
  setWidget(CVarWidget);

  setIcon(QIcon(":/GuiFoundation/Icons/CVar.svg"));

  connect(CVarWidget, &plQtCVarWidget::onBoolChanged, this, &plQtCVarsWidget::BoolChanged);
  connect(CVarWidget, &plQtCVarWidget::onFloatChanged, this, &plQtCVarsWidget::FloatChanged);
  connect(CVarWidget, &plQtCVarWidget::onIntChanged, this, &plQtCVarsWidget::IntChanged);
  connect(CVarWidget, &plQtCVarWidget::onStringChanged, this, &plQtCVarsWidget::StringChanged);

  CVarWidget->GetConsole().SetCommandInterpreter(PL_DEFAULT_NEW(plCommandInterpreterInspector));

  ResetStats();
}

void plQtCVarsWidget::ResetStats()
{
  m_CVarsBackup = m_CVars;
  m_CVars.Clear();
  CVarWidget->Clear();
}

void plQtCVarsWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage msg;

  bool bUpdateCVarsTable = false;
  bool bFillCVarsTable = false;

  while (plTelemetry::RetrieveMessage('CVAR', msg) == PL_SUCCESS)
  {
    if (msg.GetMessageID() == ' CLR')
    {
      s_pWidget->m_CVars.Clear();
    }

    if (msg.GetMessageID() == 'SYNC')
    {
      for (auto it = s_pWidget->m_CVars.GetIterator(); it.IsValid(); ++it)
      {
        auto var = s_pWidget->m_CVarsBackup.Find(it.Key());

        if (var.IsValid() && it.Value().m_uiType == var.Value().m_uiType)
        {
          it.Value().m_bValue = var.Value().m_bValue;
          it.Value().m_fValue = var.Value().m_fValue;
          it.Value().m_sValue = var.Value().m_sValue;
          it.Value().m_iValue = var.Value().m_iValue;
        }
      }

      s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);

      s_pWidget->SyncAllCVarsToServer();
    }

    if (msg.GetMessageID() == 'DATA')
    {
      plString sName;
      msg.GetReader() >> sName;

      plCVarWidgetData& sd = s_pWidget->m_CVars[sName];

      msg.GetReader() >> sd.m_sPlugin;
      msg.GetReader() >> sd.m_uiType;
      msg.GetReader() >> sd.m_sDescription;

      switch (sd.m_uiType)
      {
        case plCVarType::Bool:
          msg.GetReader() >> sd.m_bValue;
          break;
        case plCVarType::Float:
          msg.GetReader() >> sd.m_fValue;
          break;
        case plCVarType::Int:
          msg.GetReader() >> sd.m_iValue;
          break;
        case plCVarType::String:
          msg.GetReader() >> sd.m_sValue;
          break;
      }

      if (sd.m_bNewEntry)
        bUpdateCVarsTable = true;

      bFillCVarsTable = true;
    }
  }

  if (bUpdateCVarsTable)
    s_pWidget->CVarWidget->RebuildCVarUI(s_pWidget->m_CVars);
  else if (bFillCVarsTable)
    s_pWidget->CVarWidget->UpdateCVarUI(s_pWidget->m_CVars);
}

void plQtCVarsWidget::ProcessTelemetryConsole(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage msg;
  plStringBuilder tmp;

  while (plTelemetry::RetrieveMessage('CMD', msg) == PL_SUCCESS)
  {
    if (msg.GetMessageID() == 'RES')
    {
      msg.GetReader() >> tmp;
      s_pWidget->CVarWidget->AddConsoleStrings(tmp);
    }
  }
}

void plQtCVarsWidget::SyncAllCVarsToServer()
{
  for (auto it = m_CVars.GetIterator(); it.IsValid(); ++it)
    SendCVarUpdateToServer(it.Key().GetData(), it.Value());
}

void plQtCVarsWidget::SendCVarUpdateToServer(plStringView sName, const plCVarWidgetData& cvd)
{
  plTelemetryMessage Msg;
  Msg.SetMessageID('SVAR', ' SET');
  Msg.GetWriter() << sName;
  Msg.GetWriter() << cvd.m_uiType;

  switch (cvd.m_uiType)
  {
    case plCVarType::Bool:
      Msg.GetWriter() << cvd.m_bValue;
      break;

    case plCVarType::Float:
      Msg.GetWriter() << cvd.m_fValue;
      break;

    case plCVarType::Int:
      Msg.GetWriter() << cvd.m_iValue;
      break;

    case plCVarType::String:
      Msg.GetWriter() << cvd.m_sValue;
      break;
  }

  plTelemetry::SendToServer(Msg);
}

void plQtCVarsWidget::BoolChanged(plStringView sCVar, bool newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_bValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void plQtCVarsWidget::FloatChanged(plStringView sCVar, float newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_fValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void plQtCVarsWidget::IntChanged(plStringView sCVar, int newValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_iValue = newValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}

void plQtCVarsWidget::StringChanged(plStringView sCVar, plStringView sNewValue)
{
  auto& cvarData = m_CVars[sCVar];
  cvarData.m_sValue = sNewValue;
  SendCVarUpdateToServer(sCVar, cvarData);
}
