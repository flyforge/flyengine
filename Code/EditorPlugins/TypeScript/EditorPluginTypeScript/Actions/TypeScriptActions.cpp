#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorPluginTypeScript/Actions/TypeScriptActions.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptAction, 1, plRTTINoAllocator)
  ;
PL_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plTypeScriptActions::s_hCategory;
plActionDescriptorHandle plTypeScriptActions::s_hEditScript;


void plTypeScriptActions::RegisterActions()
{
  s_hCategory = PL_REGISTER_CATEGORY("TypeScriptCategory");
  s_hEditScript = PL_REGISTER_ACTION_1(
    "TypeScript.Edit", plActionScope::Document, "TypeScripts", "Edit Script", plTypeScriptAction, plTypeScriptAction::ActionType::EditScript);
}

void plTypeScriptActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hEditScript);
}

void plTypeScriptActions::MapActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "TypeScriptCategory";

  pMap->MapAction(s_hEditScript, szSubPath, 1.0f);
}

plTypeScriptAction::plTypeScriptAction(const plActionContext& context, const char* szName, plTypeScriptAction::ActionType type, float fSimSpeed)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<plTypeScriptAssetDocument*>(static_cast<const plTypeScriptAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::EditScript:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
  }
}


void plTypeScriptAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::EditScript:
      m_pDocument->EditScript();
      return;
  }
}
