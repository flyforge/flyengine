#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorPluginAi/Actions/AiActions.h>
#include <EditorPluginAi/Dialogs/AiProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAiAction, 0, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plAiActions::s_hCategoryAi;
plActionDescriptorHandle plAiActions::s_hProjectSettings;

void plAiActions::RegisterActions()
{
  s_hCategoryAi = PLASMA_REGISTER_CATEGORY("Ai");
  s_hProjectSettings = PLASMA_REGISTER_ACTION_1("Ai.Settings.Project", plActionScope::Document, "Ai", "", plAiAction, plAiAction::ActionType::ProjectSettings);
}

void plAiActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategoryAi);
  plActionManager::UnregisterAction(s_hProjectSettings);
}

void plAiActions::MapMenuActions()
{
  plActionMap* pMap = plActionMapManager::GetActionMap("EditorPluginScene_Scene2MenuBar");
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryAi, "G.Plugins.Settings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "Ai", 1.0f);
}

plAiAction::plAiAction(const plActionContext& context, const char* szName, ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/AiPlugin/plAiPlugin.svg");
      break;
  }
}

plAiAction::~plAiAction() = default;

void plAiAction::Execute(const plVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    plQtAiProjectSettingsDlg dlg(nullptr);
    if (dlg.exec() == QDialog::Accepted)
    {
      plToolsProject::BroadcastConfigChanged();
    }
  }
}
