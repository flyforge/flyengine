#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <GuiFoundation/Action/ActionManager.h>

plActionDescriptorHandle plProcGenActions::s_hCategory;
plActionDescriptorHandle plProcGenActions::s_hDumpAST;
plActionDescriptorHandle plProcGenActions::s_hDumpDisassembly;

void plProcGenActions::RegisterActions()
{
  s_hCategory = PLASMA_REGISTER_CATEGORY("ProcGen");
  s_hDumpAST =
    PLASMA_REGISTER_ACTION_1("ProcGen.DumpAST", plActionScope::Document, "ProcGen Graph", "", plProcGenAction, plProcGenAction::ActionType::DumpAST);
  s_hDumpDisassembly = PLASMA_REGISTER_ACTION_1(
    "ProcGen.DumpDisassembly", plActionScope::Document, "ProcGen Graph", "", plProcGenAction, plProcGenAction::ActionType::DumpDisassembly);
}

void plProcGenActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hDumpAST);
  plActionManager::UnregisterAction(s_hDumpDisassembly);
}

void plProcGenActions::MapMenuActions()
{
  plActionMap* pMap = plActionMapManager::GetActionMap("ProcGenAssetMenuBar");
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategory, "G.Tools.Document", 10.0f);
  pMap->MapAction(s_hDumpAST, "G.Tools.Document", "ProcGen", 1.0f);
  pMap->MapAction(s_hDumpDisassembly, "G.Tools.Document", "ProcGen", 2.0f);

  pMap = plActionMapManager::GetActionMap("ProcGenAssetToolBar");
  PLASMA_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategory, "", 12.0f);
  pMap->MapAction(s_hDumpAST, "ProcGen", 0.0f);
  pMap->MapAction(s_hDumpDisassembly, "ProcGen", 0.0f);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenAction, 0, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plProcGenAction::plProcGenAction(const plActionContext& context, const char* szName, ActionType type)
  : plButtonAction(context, szName, false, "")
  , m_Type(type)
{
}

plProcGenAction::~plProcGenAction() {}

void plProcGenAction::Execute(const plVariant& value)
{
  if (auto pAssetDocument = plDynamicCast<plProcGenGraphAssetDocument*>(GetContext().m_pDocument))
  {
    pAssetDocument->DumpSelectedOutput(m_Type == ActionType::DumpAST, m_Type == ActionType::DumpDisassembly);
  }
}
