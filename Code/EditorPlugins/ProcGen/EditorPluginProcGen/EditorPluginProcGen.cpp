#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const plToolsProjectEvent& e);

void OnLoadPlugin()
{
  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "ProcGenAssetMenuBar";

  plActionMapManager::RegisterActionMap(szMenuBar).IgnoreResult();
  plStandardMenus::MapActions(szMenuBar, plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
  plProjectActions::MapActions(szMenuBar);
  plDocumentActions::MapMenuActions(szMenuBar);
  plAssetActions::MapMenuActions(szMenuBar);
  plCommandHistoryActions::MapActions(szMenuBar);

  plEditActions::MapActions("ProcGenAssetMenuBar", false, false);
}

// Tool Bar
{
  const char* szToolBar = "ProcGenAssetToolBar";
  plActionMapManager::RegisterActionMap(szToolBar).IgnoreResult();
  plDocumentActions::MapToolbarActions(szToolBar);
  plCommandHistoryActions::MapActions(szToolBar, "");
  plAssetActions::MapToolBarActions(szToolBar, true);
}
}

// Scene
{
  // Menu Bar
  {
    plProcGenActions::RegisterActions();
    plProcGenActions::MapMenuActions();
  }

  // Tool Bar
  {
  }
}
}

void OnUnloadPlugin()
{
  plProcGenActions::UnregisterActions();
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
