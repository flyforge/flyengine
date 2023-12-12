#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltAction, 0, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plJoltActions::s_hCategoryJolt;
plActionDescriptorHandle plJoltActions::s_hProjectSettings;

void plJoltActions::RegisterActions()
{
  s_hCategoryJolt = PLASMA_REGISTER_CATEGORY("Jolt");
  s_hProjectSettings =
    PLASMA_REGISTER_ACTION_1("Jolt.Settings.Project", plActionScope::Document, "Jolt", "", plJoltAction, plJoltAction::ActionType::ProjectSettings);
}

void plJoltActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategoryJolt);
  plActionManager::UnregisterAction(s_hProjectSettings);
}

void plJoltActions::MapMenuActions()
{
  /// \todo Is there a way to integrate into ALL document types in a specific menu (ie. project settings)
  plActionMap* pMap = plActionMapManager::GetActionMap("EditorPluginScene_Scene2MenuBar");
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryJolt, "Menu.Editor/ProjectCategory/Menu.ProjectSettings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "Menu.Editor/ProjectCategory/Menu.ProjectSettings/Jolt", 1.0f);
}

plJoltAction::plJoltAction(const plActionContext& context, const char* szName, ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      // SetIconPath(":/EditorPluginScene/Icons/GizmoNone.svg"); /// \todo Icon
      break;
  }
}

plJoltAction::~plJoltAction() {}

void plJoltAction::Execute(const plVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    plQtJoltProjectSettingsDlg dlg(nullptr);
    if (dlg.exec() == QDialog::Accepted)
    {
      plToolsProject::BroadcastConfigChanged();
    }
  }
}
